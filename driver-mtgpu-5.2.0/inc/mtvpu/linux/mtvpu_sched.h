#ifndef _MTVPU_SCHED_H_
#define _MTVPU_SCHED_H_

#include "linux-types.h"

struct mt_chip;
struct mt_virm;
struct vm_elem;
struct spinlock;

#define HOST_OSID_SIZE 64

#define VCORE_INDEX 0
#define VCPU_INDEX  1
#define VCORE_ENC_SEGMENT_INDEX 2

#define MAX_CMDS_EACH_VM	2
#define VDI_TASK_DISPATCHER	0
struct vm_task_node {
	struct vm_elem *el;
	struct mt_virm *vm;
	int vm_id;

	struct list_head list;
};

int init_sched2_threads(struct mt_chip *chip);
struct mt_virm *get_next_undispatched_vm(struct mt_chip *chip, int current_vm_idx);
void task_count_add(struct mt_chip *chip, struct mt_virm *vm, int target_core);
void task_count_reduce(struct mt_chip *chip, struct mt_virm *vm, int target_core);
int put_task(struct mt_chip *chip, int target_core, struct vm_elem *el, struct mt_virm *vm,
	     int vm_id);
struct vm_task_node *get_task(struct mt_chip *chip, int target_core);
void free_task_node(struct vm_task_node *node);
int vpu_host_dispatch(void *arg);
int vpu_consume_task(struct mt_chip *chip, int thread_id);
int vpu_host_dec1(void *arg);
int vpu_host_dec2(void *arg);
int vpu_host_enc1(void *arg);
int vpu_host_enc2(void *arg);
int vpu_host_enc3(void *arg);

#endif