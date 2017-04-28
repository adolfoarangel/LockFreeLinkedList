#include <iostream>
#include <thread>
#include <atomic>
#include <threads.h>

#define SET_MARKED(p)  ((Node *)(((uintptr_t)(p)) | 1))
#define CLEAR_MARKED(p)   ((Node *)(((uintptr_t)(p)) & ~1))
#define IS_MARKED(p)  (((uintptr_t)(p)) & 1)

using namespace std;
struct Node;

typedef struct Node {
    int key;
    std::atomic<Node*> next;
} Node;

typedef struct Window {
	Node* pred;
	Node* curr;
} Window;

Node* head;

static Window* find(void *arg){
	int Num = arg;

	// initlize the nodes
	Node* pred = NULL;
	Node* curr = NULL;
	Node* succ = NULL;

	// original mark  and snip
	bool marked = false;
	bool snip = false;

	while(true){
		pred = head;
		curr = pred->next.load();
		while(true){
			succ = curr->next.load();
			marked = IS_MARKED(succ);
			while(marked){
				snip = pred->next.compare_exchange_strong(curr, succ);
				if(!snip) { goto ContinueMe;}
				curr = succ;
				succ = curr->next.load();
			}
			if(curr->key >= Num){
				Window* window = new Window;
				window->pred = pred;
				window->curr = curr;
				return window;
			}
			pred = curr;
			curr = succ;
		}
		ContinueMe: ;
	}
}

static void add(void *arg){
	while(true){
		int  Num = arg;
		Window* window = find(head, Num);

		// get are section for the add to occur
		Node* pred = window->pred;
		Node* curr = window->curr;

		if(curr->key == Num)
			return;
		else {
			// set up the new node
			Node* node = new Node;
			node->key = Num;
			node->next = curr;

			if(pred->next.compare_exchange_strong(curr, node)){ // curr may need to be changed to CLEAR_MARKED(curr)
				return;
			}
		}
	}
}

static bool remove(void *arg){
	int Num = arg;

	bool snip;
	while(true){
		Window* window = find(head, Num);
		Node* pred = window->pred;
		Node* curr = window->curr;

		if(curr->key != Num){
			return false;
		} else {
				Node* succ = curr->next;
				Node* next_marked = SET_MARKED(succ);
				snip = curr->next.compare_exchange_strong(succ, next_marked);

				if(!snip)
					continue;

				pred->next.compare_exchange_strong(curr, succ);
				return true;
		}
	}
}

static bool contains(void *arg){
	int Num = arg;

	// initial value of marked
	bool marked = false;
	//Get the head of the linked list we are going to search through
	Node* curr = head;
	//iterate through the linked list
	while(curr->key < Num){
		// go to the next node
		curr = curr->next;
		// Get succ
		Node* succ = curr->next;
		// checks if succ is marked
		marked = IS_MARKED(succ);
	}
	return (curr->key == Num && !marked);
}

int user_main(int argc, char *argv[])
{
	thrd_t thread1;
	thrd_t thread2;
	thrd_t thread3;

	Node* head = new Node;

	thrd_create(&thread1, NULL, add(), 6);
	thrd_create(&thread1, NULL, contains(), 7);
	thrd_create(&thread1, NULL, remove(), 6);

	thrd_join(thread1);
	thrd_join(thread2);
	thrd_join(thread3);

	return 0;
}
