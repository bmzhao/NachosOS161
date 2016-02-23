#include <types.h>
#include <lib.h>
#include <synchprobs.h>
#include <synch.h>
#include <opt-A1.h>


/* 
 * This simple default synchronization mechanism allows only creature at a time to
 * eat.   The globalCatMouseSem is used as a a lock.   We use a semaphore
 * rather than a lock so that this code will work even before locks are implemented.
 */

/* 
 * Replace this default synchronization mechanism with your own (better) mechanism
 * needed for your solution.   Your mechanism may use any of the available synchronzation
 * primitives, e.g., semaphores, locks, condition variables.   You are also free to 
 * declare other global variables if your solution requires them.
 */

/*
 * replace this with declarations of any synchronization and other variables you need here
 */

#ifndef OPT_A1
  #define OPT_A1
#endif

#if OPT_A1
struct lock** bowl_locks;
static volatile char *bowl_array;
char* bowl_index_chars;

struct lock* current_turn_lock;
char current_turn;

struct lock* cat_count_lock;
int num_waiting_cats;

struct lock* mice_count_lock;
int num_waiting_mice;

struct lock* mice_eating_count_lock;
int num_eating_mice;

struct lock* cat_eating_count_lock;
int num_eating_cats;

struct cv* ok_for_cats;
struct cv* ok_for_mice;

int get_mice_count(void);
int get_cat_count(void);
int increment_cat_eating(void);
int increment_mice_eating(void);
int decrement_cat_eating(void);
int decrement_mice_eating(void);
void decrement_cat_count(void);
void decrement_mice_count(void);
void increment_cat_count(void);
void increment_mice_count(void);

#define DEBUGPRINT(n) kprintf("%d\n",n);

#else
static struct semaphore *globalCatMouseSem;
#endif



/*
 * The CatMouse simulation will call this function once before any cat or
 * mouse tries to each.
 *
 * You can use it to initialize synchronization and other variables.
 * 
 * parameters: the number of bowls
 */
void
catmouse_sync_init(int bowls)
{
#if OPT_A1

    bowl_array = kmalloc(bowls*sizeof(char));
    if (bowl_array == NULL) {
        panic("initialize_bowls: unable to allocate space for %d bowls\n",bowls);
    }

    bowl_index_chars = kmalloc(bowls*sizeof(char));
    if (bowl_index_chars == NULL) {
        panic("initialize_bowls: unable to allocate space for %d bowls\n",bowls);
    }

    bowl_locks = kmalloc(bowls*sizeof(struct lock *));
    if (bowl_locks == NULL) {
        panic("initialize_bowls: unable to allocate space for %d bowls\n",bowls);
    }


    int i;
    for (i = 0; i < bowls; i++){
        bowl_array[i] = '-';
    }


    for (i = 0; i < bowls; i++){
        bowl_index_chars[i] = (char)i;
    }


    for (i = 0; i < bowls; i++){
        bowl_locks[i] = lock_create(&bowl_index_chars[i]);
        KASSERT(bowl_locks[i] != NULL);
    }

    current_turn = '-';
    current_turn_lock = lock_create("turn_lock");
    KASSERT(current_turn_lock != NULL);

    num_waiting_cats = 0;
    cat_count_lock = lock_create("cat_lock");
    KASSERT(cat_count_lock != NULL);

    num_waiting_mice = 0;
    mice_count_lock = lock_create("mice_lock");
    KASSERT(mice_count_lock != NULL);

    cat_eating_count_lock = lock_create("cat_eating_count_lock");
    KASSERT(cat_eating_count_lock != NULL);
    num_eating_cats = 0;

    mice_eating_count_lock = lock_create("mice_eating_count_lock");
    KASSERT(mice_eating_count_lock != NULL);
    num_eating_mice = 0;
    

    ok_for_cats = cv_create("ok_for_cats");
    ok_for_mice = cv_create("ok_for_mice");

    DEBUGPRINT(140)


#else
    /* replace this default implementation with your own implementation of catmouse_sync_init */

  (void)bowls; /* keep the compiler from complaining about unused parameters */
  globalCatMouseSem = sem_create("globalCatMouseSem",1);
  if (globalCatMouseSem == NULL) {
    panic("could not create global CatMouse synchronization semaphore");
  }
  return;
#endif
}

/* 
 * The CatMouse simulation will call this function once after all cat
 * and mouse simulations are finished.
 *
 * You can use it to clean up any synchronization and other variables.
 *
 * parameters: the number of bowls
 */
void
catmouse_sync_cleanup(int bowls)
{
#if OPT_A1
    if (bowl_array != NULL) {
        kfree( (void *) bowl_array );
        bowl_array = NULL;
    }

    if (bowl_index_chars != NULL) {
        kfree( (void *) bowl_index_chars);
        bowl_index_chars = NULL;
    }

    if (bowl_locks != NULL) {
        int i;
        for (i = 0; i < bowls; i++){
            lock_destroy(bowl_locks[i]);
        }
        kfree( (void *) bowl_locks);
        bowl_locks = NULL;
    }


    lock_destroy(current_turn_lock);
    lock_destroy(cat_count_lock);
    lock_destroy(mice_count_lock);
    lock_destroy(mice_eating_count_lock);
    lock_destroy(cat_eating_count_lock);

    cv_destroy(ok_for_cats);
    cv_destroy(ok_for_mice);
#else
  /* replace this default implementation with your own implementation of catmouse_sync_cleanup */
  (void)bowls; /* keep the compiler from complaining about unused parameters */
  KASSERT(globalCatMouseSem != NULL);
  sem_destroy(globalCatMouseSem);
#endif
}


/*
 * The CatMouse simulation will call this function each time a cat wants
 * to eat, before it eats.
 * This function should cause the calling thread (a cat simulation thread)
 * to block until it is OK for a cat to eat at the specified bowl.
 *
 * parameter: the number of the bowl at which the cat is trying to eat
 *             legal bowl numbers are 1..NumBowls
 *
 * return value: none
 */

void
cat_before_eating(unsigned int bowl)
{
#if OPT_A1
    bowl--;
    char local_current_turn;
    KASSERT(current_turn_lock != NULL);

    lock_acquire(current_turn_lock); //we've acquired turn_lock
    local_current_turn = current_turn;

    DEBUGPRINT(226)

    if (local_current_turn == 'c'){
        int mouse_waiting_count = get_mice_count();
        if (mouse_waiting_count > 0){
            //wait until mice are done before approaching the problem
            increment_cat_count();
            cv_wait(ok_for_cats, current_turn_lock);
            decrement_cat_count();

            lock_release(current_turn_lock); //we released current turn lock

            //get the lock for the bowl
            lock_acquire(bowl_locks[bowl]);
            bowl_array[bowl] = 'c';
        } else {
            KASSERT(mouse_waiting_count == 0);
            //no mice waiting
            //cat should be able to eat, if bowl is open
            lock_release(current_turn_lock); // we released current turn lock

             //get the lock for the bowl
            lock_acquire(bowl_locks[bowl]);
            bowl_array[bowl] = 'c';
        }
    } else if (local_current_turn == 'm') {
        increment_cat_count();
        cv_wait(ok_for_cats, current_turn_lock);
        decrement_cat_count();
        lock_release(current_turn_lock); //w-e released current turn lock

        //try to eat from bowl
        lock_acquire(bowl_locks[bowl]);
        bowl_array[bowl] = 'c';

    } else {
        //if current turn == '-'

        current_turn = 'c';
        lock_release(current_turn_lock);

        lock_acquire(bowl_locks[bowl]); //we released current turn lock
        bowl_array[bowl] = 'c';
    }
    increment_cat_eating();
    
    //if cat turn
        //if num mice waiting > 0
            //increment num_cats_waiting
            //wait on condition variable of "ok_for_cat"
            //decrement num_cats_waiting
            //try to eat from bowl
        //if num mice waiting == 0
            //try to eat from bowl

    //else if mouse turn
        //increment num_cats_waiting
        //wait on condition variable of "ok_for_cat"
        //decrement num_cats_waiting
        //try to eat from bowl

    //else dash turn
       //set turn variable to 'c'
        //get your bowl

#else
  /* replace this default implementation with your own implementation of cat_before_eating */
  (void)bowl;  /* keep the compiler from complaining about an unused parameter */
  KASSERT(globalCatMouseSem != NULL);
  P(globalCatMouseSem);
#endif
}

/*
 * The CatMouse simulation will call this function each time a cat finishes
 * eating.
 *
 * You can use this function to wake up other creatures that may have been
 * waiting to eat until this cat finished.
 *
 * parameter: the number of the bowl at which the cat is finishing eating.
 *             legal bowl numbers are 1..NumBowls
 *
 * return value: none
 */

void
cat_after_eating(unsigned int bowl)
{
#if OPT_A1
    bowl--;
    char local_current_turn;
    KASSERT(current_turn_lock != NULL);
    lock_acquire(current_turn_lock); //we've acquired turn_lock
    local_current_turn = current_turn;

    KASSERT(local_current_turn == 'c');
    int cat_number = decrement_cat_eating();

    if (cat_number == 0) {
        //release the lock
        lock_release(bowl_locks[bowl]);
        bowl_array[bowl] = '-';
        cv_broadcast(ok_for_mice,current_turn_lock);
        current_turn = '-';
        lock_release(current_turn_lock);
    } else {
        //release the lock
        lock_release(bowl_locks[bowl]);
        bowl_array[bowl] = '-';
        lock_release(current_turn_lock);
    }

    //Pseudocode
    //kassert that its cat turn

    //At the very beginning, decrement cat eating
    //if result was == 0, we know it was last cat
        //release your bowl
        //acquire the turn lock
        //broadcast ok_for_mice
        //change the turn lock to '-'
        //release turn lock

    //else not last cat
        //release your bowl
        //set your bowl to '-'
#else
  /* replace this default implementation with your own implementation of cat_after_eating */
  (void)bowl;  /* keep the compiler from complaining about an unused parameter */
  KASSERT(globalCatMouseSem != NULL);
  V(globalCatMouseSem);
#endif
}

/*
 * The CatMouse simulation will call this function each time a mouse wants
 * to eat, before it eats.
 * This function should cause the calling thread (a mouse simulation thread)
 * to block until it is OK for a mouse to eat at the specified bowl.
 *
 * parameter: the number of the bowl at which the mouse is trying to eat
 *             legal bowl numbers are 1..NumBowls
 *
 * return value: none
 */

void
mouse_before_eating(unsigned int bowl)
{
#if OPT_A1
    bowl--;
    char local_current_turn;
    KASSERT(current_turn_lock != NULL);

    DEBUGPRINT(379)

    lock_acquire(current_turn_lock); //we've acquired turn_lock
    local_current_turn = current_turn;

    if (local_current_turn == 'm'){
        int cat_waiting_count = get_cat_count();
        if (cat_waiting_count > 0){
            //wait until cats are done before approaching the problem
            increment_mice_count();
            cv_wait(ok_for_mice, current_turn_lock);
            decrement_mice_count();

            lock_release(current_turn_lock); //we released current turn lock

            //get the lock for the bowl
            lock_acquire(bowl_locks[bowl]);
            bowl_array[bowl] = 'm';

        } else {
            KASSERT(cat_waiting_count == 0);
            //no cats waiting
            //mice should be able to eat, if bowl is open
            lock_release(current_turn_lock); // we released current turn lock

             //get the lock for the bowl
            lock_acquire(bowl_locks[bowl]);
            bowl_array[bowl] = 'm';
        }
    } else if (local_current_turn == 'c') {
        increment_mice_count();
        cv_wait(ok_for_mice, current_turn_lock);
        decrement_mice_count();
        lock_release(current_turn_lock); //w-e released current turn lock

        //try to eat from bowl
        lock_acquire(bowl_locks[bowl]);
        bowl_array[bowl] = 'm';

    } else {
        //if current turn == '-'

        current_turn = 'm';
        lock_release(current_turn_lock);

        lock_acquire(bowl_locks[bowl]); //we released current turn lock
        bowl_array[bowl] = 'm';
    }
    increment_mice_eating();
#else
  /* replace this default implementation with your own implementation of mouse_before_eating */
  (void)bowl;  /* keep the compiler from complaining about an unused parameter */
  KASSERT(globalCatMouseSem != NULL);
  P(globalCatMouseSem);
#endif
}

/*
 * The CatMouse simulation will call this function each time a mouse finishes
 * eating.
 *
 * You can use this function to wake up other creatures that may have been
 * waiting to eat until this mouse finished.
 *
 * parameter: the number of the bowl at which the mouse is finishing eating.
 *             legal bowl numbers are 1..NumBowls
 *
 * return value: none
 */

void
mouse_after_eating(unsigned int bowl)
{
#if OPT_A1
    bowl--;
    char local_current_turn;
    KASSERT(current_turn_lock != NULL);
    lock_acquire(current_turn_lock); //we've acquired turn_lock
    local_current_turn = current_turn;

    KASSERT(local_current_turn == 'm');
    int mice_number = decrement_mice_eating();

    if (mice_number == 0) {
        //release the lock
        lock_release(bowl_locks[bowl]);
        bowl_array[bowl] = '-';
        cv_broadcast(ok_for_cats,current_turn_lock);
        current_turn = '-';
        lock_release(current_turn_lock);
    } else {
        //release the lock
        lock_release(bowl_locks[bowl]);
        bowl_array[bowl] = '-';
        lock_release(current_turn_lock);
    }
#else
  /* replace this default implementation with your own implementation of mouse_after_eating */
  (void)bowl;  /* keep the compiler from complaining about an unused parameter */
  KASSERT(globalCatMouseSem != NULL);
  V(globalCatMouseSem);
#endif
}

#if OPT_A1
int get_mice_count(void){
    KASSERT(mice_count_lock != NULL);
    lock_acquire(mice_count_lock);
    int return_value = num_waiting_mice;
    lock_release(mice_count_lock);
    return return_value;
}

int get_cat_count(void) {
    KASSERT(cat_count_lock != NULL);
    lock_acquire(cat_count_lock);
    int return_value = num_waiting_cats;
    lock_release(cat_count_lock);
    return return_value;
}

int increment_cat_eating(void) {
    KASSERT(cat_eating_count_lock != NULL);
    lock_acquire(cat_eating_count_lock);
    num_eating_cats++;
    int return_value = num_eating_cats;
    lock_release(cat_eating_count_lock);
    return return_value;
}

int decrement_cat_eating(void) {
    KASSERT(cat_eating_count_lock != NULL);
    lock_acquire(cat_eating_count_lock);
    num_eating_cats--;
    int return_value = num_eating_cats;
    lock_release(cat_eating_count_lock);
    return return_value;
}

int increment_mice_eating(void) {
    KASSERT(mice_eating_count_lock != NULL);
    lock_acquire(mice_eating_count_lock);
    num_eating_mice++;
    int return_value = num_eating_mice;
    lock_release(mice_eating_count_lock);
    return return_value;
}

int decrement_mice_eating(void) {
    KASSERT(mice_eating_count_lock != NULL);
    lock_acquire(mice_eating_count_lock);
    num_eating_mice--;
    int return_value = num_eating_mice;
    lock_release(mice_eating_count_lock);
    return return_value;
}

void increment_mice_count(void){
    KASSERT(mice_count_lock != NULL);
    lock_acquire(mice_count_lock);
    num_waiting_mice++;
    lock_release(mice_count_lock);
}

void increment_cat_count(void){
    KASSERT(cat_count_lock != NULL);
    lock_acquire(cat_count_lock);
    num_waiting_cats++;
    lock_release(cat_count_lock);
}

void decrement_mice_count(void) {
    KASSERT(mice_count_lock != NULL);
    lock_acquire(mice_count_lock);
    num_waiting_mice--;
    lock_release(mice_count_lock);
}

void decrement_cat_count(void) {
    KASSERT(cat_count_lock != NULL);
    lock_acquire(cat_count_lock);
    num_waiting_cats--;
    lock_release(cat_count_lock);
}
#else
#endif
