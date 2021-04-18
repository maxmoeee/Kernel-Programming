/* FILE: kernel/sched/ts.c */

#include "sched.h"

#define MAX_TS_PRIO 40

int get_new_prio(int prio, int type)
{
	int new_prio;
	// type: 0(expire), 1(sleep)
	prio = 139 - prio;
	if (prio < 0 || prio > 39) {
		printk(KERN_INFO "[ts scheduler] Invalid priority number!\n");
		return 139;
	}

	if (type == 0) {
		if (prio < 10)
			new_prio = 0;
		else if (prio < 40)
			new_prio = prio - 10;
	} else {
		if (prio < 10)
			new_prio = 30;
		else if (prio < 20)
			new_prio = 31;
		else if (prio < 25)
			new_prio = prio + 12;
		else if (prio < 27)
			new_prio = prio + 11;
		else if (prio == 27)
			new_prio = 37;
		else if (prio < 30)
			new_prio = 38;
		else if (prio < 40)
			new_prio = 39;
	}

	return 139 - new_prio;
}

int get_num_ticks(int prio)
{
	prio = 139 - prio;
	if (prio < 0)
		goto quit;

	if (prio < 10)
		return 16;
	if (prio < 20)
		return 12;
	if (prio < 30)
		return 8;
	if (prio < 40)
		return 4;

quit:
	printk(KERN_INFO "[ts scheduler] Invalid priority number!\n");
	return 0;
}


void init_ts_rq(struct ts_rq *ts_rq)
{
	/* Equivalent to an empty doubly linked list:
		rr_rq->queue.prev = &rr_rq->queue;
		rr_rq->queue.next = &rr_rq->queue;  */
	int i;
	for (i = 0; i < MAX_TS_PRIO; i++)
		INIT_LIST_HEAD(ts_rq->queue + i);
}

static void
enqueue_task_ts(struct rq *rq, struct task_struct *p, int flags)
{
	struct sched_ts_entity *ts_se = &p->ts;
	int idx = 139 - p->prio;

	if (flags & ENQUEUE_WAKEUP) { // change priority [case 2]
		p->prio = get_new_prio(p->prio, 1);
	}

	list_add_tail(&ts_se->list, rq->ts.queue + idx);

	printk(KERN_INFO"[SCHED_ts] ENQUEUE: p->pid=%d, p->policy=%d "
		"curr->pid=%d, curr->policy=%d, flags=%d\n",
		p->pid, p->policy, rq->curr->pid, rq->curr->policy, flags);
}

static void
dequeue_task_ts(struct rq *rq, struct task_struct *p, int flags)
{
	struct sched_ts_entity *ts_se = &p->ts;

	list_del(&ts_se->list);

	printk(KERN_INFO"[SCHED_ts] DEQUEUE: p->pid=%d, p->policy=%d "
			"curr->pid=%d, curr->policy=%d, flags=%d\n",
		p->pid, p->policy, rq->curr->pid, rq->curr->policy, flags);
}

static void yield_task_ts(struct rq *rq)
{
	struct sched_ts_entity *ts_se = &rq->curr->ts;
	struct ts_rq *ts_rq = &rq->ts;

	int idx = 139 - rq->curr->prio;
	// yield the current task, put it to the end of the current queue
	list_move_tail(&ts_se->list, ts_rq->queue + idx);

	printk(KERN_INFO"[SCHED_ts] YIELD: Process-%d\n", rq->curr->pid);
}

static void
check_preempt_curr_ts(struct rq *rq, struct task_struct *p, int flags)
{
	/* Check whether rq->curr should be preempted by the newly woken task p
	   If rq->curr needs to be preempted, then set the TIF_NEED_RESCHED flag of rq->curr */
	if (rq->curr->prio > p->prio)
		resched_curr(rq);
	return;
}

static struct task_struct *
pick_next_task_ts(struct rq *rq, struct task_struct *prev, struct rq_flags *rf)
{
	struct sched_ts_entity *ts_se = NULL;
	struct task_struct *p = NULL;
	struct ts_rq *ts_rq = &rq->ts;

	int i;
	for (i = MAX_TS_PRIO - 1; i >= 0; i--) // from lowest prio to highest
	{
		if (!list_empty(ts_rq->queue + i)) {
			ts_se = list_entry((ts_rq->queue)[i].next, struct sched_ts_entity, list);
			break;
		}
	}

	if (!ts_se) return NULL; // All the queues are empty
	put_prev_task(rq, prev);

	p = container_of(ts_se, struct task_struct, ts);
	return p;
}

static void put_prev_task_ts(struct rq *rq, struct task_struct *p)
{
	/* Not implemented in ts */
}

static void set_curr_task_ts(struct rq *rq)
{
	/* Not implemented in ts */
}

static void task_tick_ts(struct rq *rq, struct task_struct *p, int queued)
{
	int idx;

	if (p->policy != SCHED_TS)
		return;

	printk(KERN_INFO"[SCHED_TS] TASK TICK: Process-%d = %d\n", p->pid, p->ts.time_slice);

	/* Case 1:
	Decrease time_slice first. If time_slice > 0, keep running */
	if(--p->ts.time_slice)
		return;

	/* Case 2:
	time_slice equals to 0

		1. change the priority & timeslice accordingly
		2. move the entity to its new corresponding queue
		3. set the need_reched flag (No need for optimization)

	*/
	p->prio = get_new_prio(p->prio, 0); // change priority [case 1]
	p->ts.time_slice = get_num_ticks(p->prio);
	printk(KERN_INFO "[ts scheduler] process %d's priority chaned to %d.\n", p->pid, p->prio);

	idx = 139 - p->prio;
	list_move_tail(&p->ts.list, rq->ts.queue + idx);

	resched_curr(rq);
}

unsigned int get_rr_interval_ts(struct rq *rq, struct task_struct *p)
{
	// return the default time slice of the task under the current priority
	return get_num_ticks(p->prio);
}

static void
prio_changed_ts(struct rq *rq, struct task_struct *p, int oldprio)
{
	/* priority won't be explicity changed: nothing to do */
}

static void switched_to_ts(struct rq *rq, struct task_struct *p)
{
	/* priority won't be explicity changed: nothing to do */
}

static void update_curr_ts(struct rq *rq)
{
	/* nothing to do */
}

// Minimal support to SMP (assume single core CPU)
#ifdef CONFIG_SMP
static int
select_task_rq_ts(struct task_struct *p, int cpu, int sd_flag, int flags)
{
	return task_cpu(p); /* ts tasks never migrate */
}
#endif

const struct sched_class ts_sched_class = {
	.next = &fair_sched_class,
	.enqueue_task = enqueue_task_ts,
	.dequeue_task = dequeue_task_ts,
	.yield_task = yield_task_ts,
	.check_preempt_curr = check_preempt_curr_ts,
	.pick_next_task = pick_next_task_ts,
	.put_prev_task = put_prev_task_ts,
	.set_curr_task = set_curr_task_ts,
	.task_tick = task_tick_ts,
	.get_rr_interval = get_rr_interval_ts,
	.prio_changed = prio_changed_ts,
	.switched_to = switched_to_ts,
	.update_curr = update_curr_ts,
#ifdef CONFIG_SMP
	.select_task_rq = select_task_rq_ts,
	.set_cpus_allowed = set_cpus_allowed_common,
#endif
};
