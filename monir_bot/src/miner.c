#include "bot.h"

/* Keep claims across ticks, including while a miner is cooling down. Store
 * object IDs rather than library pointers, which may disappear between ticks. */
typedef struct s_miner_job
{
	unsigned long unit_id;
	unsigned long target_id;
	t_pos site;
	unsigned int blocked;
	bool returning;
	struct s_miner_job *next;
} t_miner_job;

static t_miner_job *g_jobs;

static bool is_miner(const t_obj *o)
{
	return o && o->type == OBJ_UNIT && o->s_unit.team_id == game.my_team_id &&
		o->s_unit.name && strcmp(o->s_unit.name, UNIT_NAME_MINER) == 0;
}

static t_miner_job *get_job(unsigned long id)
{
	for (t_miner_job *j = g_jobs; j; j = j->next)
		if (j->unit_id == id)
			return j;
	t_miner_job *job = calloc(1, sizeof(*job));
	if (!job)
		return NULL;
	job->unit_id = id;
	job->next = g_jobs;
	g_jobs = job;
	return job;
}

static bool is_resource(const t_obj *o)
{
	return o && (o->type == OBJ_DEPOSIT || o->type == OBJ_GEM_PILE) &&
		o->s_deposit_gems_pile.gems > 0;
}

static bool claimed(unsigned long id, const t_miner_job *self)
{
	for (t_miner_job *j = g_jobs; j; j = j->next)
		if (j != self && j->target_id == id)
			return true;
	return false;
}

static void update_jobs(t_obj **units)
{
	t_miner_job **link = &g_jobs;
	while (*link)
	{
		t_miner_job *job = *link;
		t_obj *u = core_get_obj_from_id(job->unit_id);
		if (!is_miner(u))
		{
			*link = job->next;
			free(job);
			continue;
		}
		job->returning = u->s_unit.gems > 0;
		if (job->returning)
			job->target_id = 0;
		else if (job->target_id && !is_resource(core_get_obj_from_id(job->target_id)))
		{
			/* Finish this deposit's loose gems before choosing another site. */
			t_obj *pile = core_get_obj_from_pos(job->site);
			job->target_id = is_resource(pile) && !ft_runner_can_collect(pile, ft_get_core_own()) ? pile->id : 0;
		}
		link = &job->next;
	}
	for (int i = 0; units && units[i]; i++)
		if (is_miner(units[i]))
			get_job(units[i]->id);
}

static void assign_job(t_miner_job *job, const t_obj *u, const t_obj *core)
{
	t_obj *best = NULL;
	long best_score = LONG_MIN;
	for (int i = 0; game.objects && game.objects[i]; i++)
	{
		t_obj *o = game.objects[i];
		if (!ft_is_harvest_resource(o, core) || claimed(o->id, job) ||
			(o->type == OBJ_GEM_PILE && ft_runner_can_collect(o, core)))
			continue;
		if (!nav_can_reach(u, o->pos, true, true))
			continue;
		long score = 1000 - (long)ft_distance(u->pos, o->pos) * 25 -
			(long)ft_distance(core->pos, o->pos) * 12;
		t_obj *enemy_core = ft_get_core_opponent();
		if (enemy_core && ft_distance(enemy_core->pos, o->pos) <= ft_distance(core->pos, o->pos))
			score -= 250;
		if (o->type == OBJ_DEPOSIT)
		{
			score -= 180 + (long)o->hp * 3;
			/* With a runner, generate full deposit hauls first. Locked loose
			 * gems remain fallback work rather than starving both workers. */
			if (core_get_units_byName_count(UNIT_NAME_RUNNER))
				score += 10000;
		}
		/* Exposed miners should prefer the safe side of the map. */
		for (int e = 0; game.objects && game.objects[e]; e++)
		{
			const t_obj *enemy = game.objects[e];
			if (enemy->type == OBJ_UNIT && enemy->s_unit.team_id != game.my_team_id &&
				enemy->s_unit.properties.damage_unit > 0 && ft_distance(enemy->pos, o->pos) <= 3)
				score -= 400;
		}
		if (score > best_score)
		{
			best_score = score;
			best = o;
		}
	}
	if (best)
	{
		job->target_id = best->id;
		job->site = best->pos;
		job->blocked = 0;
	}
}

static void run_job(t_miner_job *job, t_obj *u, t_obj *core)
{
	if (u->s_unit.gems > 0)
	{
		core_debug_addObjectInfo(u, "RETURN %lu gems to Core", u->s_unit.gems);
		if (u->s_unit.action_cooldown > 0)
			return;
		if (ft_distance(u->pos, core->pos) == 1)
			core_action_transferGems(u, core->pos, u->s_unit.gems);
		else
			nav_move_adjacent_to(u, core->pos, false);
		return;
	}
	t_obj *target = core_get_obj_from_id(job->target_id);
	if (!target)
	{
		core_debug_addObjectInfo(u, "IDLE no safe reachable resource");
		if (u->s_unit.action_cooldown <= 0 && ft_distance(u->pos, core->pos) == 1)
			for (unsigned short y = 0; y < game.grid_size; y++)
				for (unsigned short x = 0; x < game.grid_size; x++)
				{
					t_pos p = {x, y};
					if (ft_distance(p, core->pos) == 2 && !core_get_obj_from_pos(p) &&
						!nav_is_reserved(p) && nav_move_to_exact(u, p, false))
						return;
				}
		return;
	}
	core_debug_addObjectInfo(u, "%s target #%lu (%u,%u)",
		target->type == OBJ_DEPOSIT ? "MINE" : "COLLECT", target->id, target->pos.x, target->pos.y);
	if (u->s_unit.action_cooldown > 0)
		return;
	if (ft_distance(u->pos, target->pos) == 1)
	{
		core_action_attack(u, target);
		job->blocked = 0;
	}
	else if (nav_move_adjacent_to(u, target->pos, false))
		job->blocked = 0;
	else if (++job->blocked >= 4)
	{
		/* Release an inaccessible job instead of reserving it forever. */
		job->target_id = 0;
		job->blocked = 0;
	}
}

void handle_miners(t_obj *core, t_obj **units)
{
	if (!core)
		return;
	update_jobs(units);
	/* Allocate before issuing actions: even cooling-down units retain claims. */
	for (int i = 0; units && units[i]; i++)
	{
		t_obj *u = units[i];
		if (!is_miner(u))
			continue;
		t_miner_job *job = get_job(u->id);
		if (job && job->target_id &&
				(!ft_is_harvest_resource(core_get_obj_from_id(job->target_id), core) ||
				(core_get_obj_from_id(job->target_id)->type == OBJ_GEM_PILE &&
				 ft_runner_can_collect(core_get_obj_from_id(job->target_id), core))))
				job->target_id = 0;
		if (job && !job->returning && !job->target_id)
			assign_job(job, u, core);
	}
	/* Unloaders get first choice of the approaches to our Core. */
	for (int pass = 0; pass < 2; pass++)
		for (int i = 0; units && units[i]; i++)
		{
			t_obj *u = units[i];
			if (!is_miner(u) || ((u->s_unit.gems > 0) != (pass == 0)))
				continue;
			t_miner_job *job = get_job(u->id);
			if (job)
				run_job(job, u, core);
		}
}
