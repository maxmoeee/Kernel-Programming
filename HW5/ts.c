/* FILE: kernel/sched/ts.c */

#include "sched.h"

void init_ts_rq(struct ts_rq *ts_rq)
{
	INIT_LIST_HEAD(&ts_rq->queue);
}

static void
enqueue_task_ts(struct rq *rq, struct task_struct *p, int flags)
{
	struct sched_ts_entity *ts_se = &p->ts;

	list_add_tail(&ts_se->list, &rq->ts.queue);

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

	// yield the current task, put it to the end of the queue
	list_move_tail(&ts_se->list, &ts_rq->queue);

	printk(KERN_INFO"[SCHED_ts] YIELD: Process-%d\n", rq->curr->pid);
}

static void
check_preempt_curr_ts(struct rq *rq, struct task_struct *p, int flags)
{
	return; // ts tasks are never preempted
}

static struct task_struct *
pick_next_task_ts(struct rq *rq, struct task_struct *prev, struct rq_flags *rf)
{
	struct sched_ts_entity *ts_se = NULL;
	struct task_struct *p = NULL;
	struct ts_rq *ts_rq = &rq->ts;
	if (list_empty(&ts_rq->queue))
		return NULL;

	put_prev_task(rq, prev);
	ts_se = list_entry(ts_rq->queue.next, struct sched_ts_entity, list);
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
	/* The function scheduler_tick() is periodically called by the kernel with the frequency HZ,
	which is the tick rate of the system timer defined on system boot.

	scheduler_tick() calls the current process's sched_class->task_tick(). For example:

		void scheduler_tick(void)
		{
			int cpu = smp_processor_id();
			struct rq *rq = cpu_rq(cpu);
			struct task_struct *curr = rq->curr;
			...
			update_rq_clock(rq);
			curr->sched_class->task_tick(rq, curr, 0);
			...
		}
	*/

	return; /* ts has no timeslice management */
}

unsigned int get_rr_interval_ts(struct rq *rq, struct task_struct *p)
{
	/* ts has no timeslice management */
	return 0;
}

static void
prio_changed_ts(struct rq *rq, struct task_struct *p, int oldprio)
{
	return; /* ts doesn't support priority */
}

static void switched_to_ts(struct rq *rq, struct task_struct *p)
{
	/* nothing to do */
}

static void update_curr_ts(struct rq *rq)
{
	/* nothing to do */
}

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
