#include <am.h>
#include <klib.h>
#include <rtthread.h>

static rt_thread_t current;

typedef struct{
	rt_ubase_t from;
	rt_ubase_t to;
	rt_bool_t has_from;
} SwContext;

static Context* ev_handler(Event e, Context *c) {
  switch (e.event) {
	  case EVENT_YIELD : 
			SwContext *ct = (SwContext *)current->user_data;
			//printf("*to = %x, to = %x, from = %x, has = %s\n", *(Context **)(ct->to), ct->to, ct->from, ct->has_from ? "true" : "false");
		  Context **from;	
		  Context **to;	
			to = (Context **)(ct->to);
			if(ct->has_from) {
				from  = (Context **)(ct->from);
			} 
			if(ct->has_from){
				*from = c;
			}
			return *to;
	  case EVENT_IRQ_TIMER: break;
    default: printf("Unhandled event ID = %d\n", e.event); assert(0);
  }
  return c;
}

void __am_cte_init() {
  cte_init(ev_handler);
}

void rt_hw_context_switch_to(rt_ubase_t to) {
	//printf("to = %x\n", to);
	current = rt_thread_self();
	rt_ubase_t tmp = current->user_data; 
	SwContext ct;
	ct.to   = to;
	ct.has_from = 0;
	//printf("to = %x, from = %x\n", ct.to, ct.from);
	current->user_data = (rt_ubase_t)&ct;
	yield();
	current->user_data = tmp;
}

void rt_hw_context_switch(rt_ubase_t from, rt_ubase_t to) {
	current = rt_thread_self();
	rt_ubase_t tmp = current->user_data; 
	SwContext ct;
	ct.from = from;
	ct.to   = to;
	ct.has_from = 1;
	current->user_data = (rt_ubase_t)&ct;
	yield();
	current->user_data = tmp;
}

void rt_hw_context_switch_interrupt(void *context, rt_ubase_t from, rt_ubase_t to, struct rt_thread *to_thread) {
  assert(0);
}

typedef struct{
	void (*tentry)(void* );
	void *parameter;
	void (*texit)(void);
} EntryPack;

void tentry_pack(void *entrypack) {
	EntryPack *pack = (EntryPack *)entrypack;
	(pack->tentry)(pack->parameter);	
	(pack->texit)();
}

rt_uint8_t *rt_hw_stack_init(void *tentry, void *parameter, rt_uint8_t *stack_addr, void *texit) {
	uintptr_t sp = (uintptr_t)stack_addr & -sizeof(uintptr_t);

  EntryPack *pack = (EntryPack *)((sp - sizeof(EntryPack)) & -sizeof(uintptr_t));
	pack->tentry = tentry;
	pack->parameter = parameter;
	pack->texit = texit;

	rt_uint8_t *c = (rt_uint8_t *)kcontext((Area) {(void *)stack_addr, (void *)pack}, tentry_pack, (void *)pack);
  return c;
}
