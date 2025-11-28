#include "BENSCHILLIBOWL.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

bool IsEmpty(BENSCHILLIBOWL* bcb);
bool IsFull(BENSCHILLIBOWL* bcb);
void AddOrderToBack(Order **orders, Order *order);

MenuItem BENSCHILLIBOWLMenu[] = { 
    "BensChilli", 
    "BensHalfSmoke", 
    "BensHotDog", 
    "BensChilliCheeseFries", 
    "BensShake",
    "BensHotCakes",
    "BensCake",
    "BensHamburger",
    "BensVeggieBurger",
    "BensOnionRings",
};

int BENSCHILLIBOWLMenuLength = 10;

/* Selects and returns a random menu item */
MenuItem PickRandomMenuItem() {
    int index = rand() % BENSCHILLIBOWLMenuLength;
    return BENSCHILLIBOWLMenu[index];
}

/* Allocates and initializes a restaurant structure with mutexes and condition variables */
BENSCHILLIBOWL* OpenRestaurant(int max_size, int expected_num_orders) {

    // Allocate memory for the restaurant
    BENSCHILLIBOWL* bcb = malloc(sizeof(BENSCHILLIBOWL));

    // Store queue size limits and expected order count
    bcb->max_size = max_size;
    bcb->expected_num_orders = expected_num_orders;

    // Initialize queue to empty
    bcb->orders = NULL;
    bcb->current_size = 0;

    // Track processed orders and next order number
    bcb->orders_handled = 0;
    bcb->next_order_number = 1;

    // Initialize mutex and condition variables
    pthread_mutex_init(&bcb->mutex, NULL);
    pthread_cond_init(&bcb->can_add_orders, NULL);
    pthread_cond_init(&bcb->can_get_orders, NULL);

    printf("Restaurant is open!\n");
    return bcb;
}

/* Closes the restaurant and ensures all expected orders were processed */
void CloseRestaurant(BENSCHILLIBOWL* bcb) {

    // Destroy mutex and condition variables
    pthread_mutex_destroy(&bcb->mutex);
    pthread_cond_destroy(&bcb->can_add_orders);
    pthread_cond_destroy(&bcb->can_get_orders);

    // Ensure all orders were handled before shutdown
    assert(bcb->orders_handled == bcb->expected_num_orders);

    // Free restaurant memory
    free(bcb);

    printf("Restaurant is closed!\n");
}

/* Adds an order to the queue and returns the assigned order number */
int AddOrder(BENSCHILLIBOWL* bcb, Order* order) {

    // Lock the shared structure
    pthread_mutex_lock(&bcb->mutex);

    // Wait until space is available in the queue
    while (IsFull(bcb)) {
        pthread_cond_wait(&bcb->can_add_orders, &bcb->mutex);
    }

    // Assign a unique order number safely inside the critical section
    order->order_number = bcb->next_order_number++;
    
    // Order will be added at the end of the queue
    order->next = NULL;

    // Insert order at back of queue
    AddOrderToBack(&bcb->orders, order);

    // Update queue size
    bcb->current_size++;

    // Signal cooks that an order is available
    pthread_cond_signal(&bcb->can_get_orders);

    // Unlock the shared structure
    pthread_mutex_unlock(&bcb->mutex);

    // Return the assigned order number
    return order->order_number;
}

/* Removes and returns the next order from the queue */
Order *GetOrder(BENSCHILLIBOWL* bcb) {

    // Lock the restaurant structure
    pthread_mutex_lock(&bcb->mutex);

    // Wait while the queue is empty
    while (IsEmpty(bcb)) {

        // If all expected orders have been handled, stop taking orders
        if (bcb->orders_handled >= bcb->expected_num_orders) {
            pthread_mutex_unlock(&bcb->mutex);
            return NULL;
        }

        // Otherwise wait for new orders to be added
        pthread_cond_wait(&bcb->can_get_orders, &bcb->mutex);
    }

    // Remove first order from the queue
    Order* order = bcb->orders;
    bcb->orders = order->next;

    // Update size and handled count
    bcb->current_size--;
    bcb->orders_handled++;

    // Signal customers that a slot has opened in the queue
    pthread_cond_signal(&bcb->can_add_orders);

    // Unlock the structure
    pthread_mutex_unlock(&bcb->mutex);

    return order;
}

/* Returns true if queue is empty */
bool IsEmpty(BENSCHILLIBOWL* bcb) {
    return bcb->current_size == 0;
}

/* Returns true if queue is full */
bool IsFull(BENSCHILLIBOWL* bcb) {
    return bcb->current_size >= bcb->max_size;
}

/* Inserts an order at the back of the linked-list queue */
void AddOrderToBack(Order **orders, Order *order) {

    // If queue is empty, new order becomes head
    if (*orders == NULL) {
        *orders = order;
    } 
    
    // Otherwise find the end and attach
    else {
        Order *curr = *orders;
        while (curr->next != NULL) {
            curr = curr->next;
        }
        curr->next = order;
    }
}
