//#pragma GCC optimize("Ofast")

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <assert.h>

#define MAX_HASH 8192
#define BASIC_ALLOC 128
#define BITS_INT ((sizeof(int) * 8))

long unsigned int max;

typedef struct
{
	int* data;
	int num_ints;
	int num_bits;
}vector_bool;

vector_bool acc_states;

void set_bit_n(unsigned int pos)
{
	if (pos > acc_states.num_bits)
	{
		unsigned int to_add = (pos - acc_states.num_bits) / BITS_INT + 1;
		acc_states.data = realloc(acc_states.data, (acc_states.num_ints+to_add)*sizeof(unsigned int));
		bzero(&acc_states.data[acc_states.num_ints], (to_add * sizeof(int)));
		acc_states.num_ints+=to_add;
		acc_states.num_bits = acc_states.num_ints * BITS_INT;
	}
	unsigned int div = pos / BITS_INT;
	unsigned int rem = pos % BITS_INT;
	acc_states.data[div] |= (1 << rem);
}

unsigned int retrieve_bit_n(unsigned int pos)
{
	if (pos > acc_states.num_bits)
		return 0;
	unsigned int div = pos / BITS_INT;
	unsigned int rem = pos % BITS_INT;
	unsigned int ret = acc_states.data[div] & (1 << rem);
	return ret != 0;
}

typedef struct
{
	char write;
	char mov;
	int final;
}transition;

typedef struct
{
	transition* data;
	int size;
}vector_transition;

vector_transition new_vector_transition()
{
	vector_transition rtn;
	rtn.data = NULL;
	rtn.size = 0;
	return rtn;
}

char app_trans_mov;
char app_trans_write;
int app_trans_final;
void append_transition(vector_transition* vect)
{
	if (app_trans_mov == 'S')
		app_trans_mov=0;
	else if (app_trans_mov == 'L')
		app_trans_mov=-1;	
	else if (app_trans_mov == 'R')
		app_trans_mov=+1;
	vect->size++;
	vect->data = realloc(vect->data, vect->size*sizeof(transition));
	vect->data[vect->size-1].mov = app_trans_mov; 
	vect->data[vect->size-1].write = app_trans_write; 
	vect->data[vect->size-1].final = app_trans_final+1;
}

void delete_vector_transition(vector_transition* vect)
{
	free(vect->data);
}

typedef struct
{
	int state;
	vector_transition exiting;
}node;

node new_node(int state)
{
	node nodo;
	nodo.state = state;
	nodo.exiting = new_vector_transition();
	return nodo;
}

typedef struct
{
	node* data;
	int size;
}vector_node;

vector_node new_vector_node()
{
	vector_node rtn;
	rtn.data = NULL;
	rtn.size = 0;
	return rtn;
}

void append_node(vector_node* vect, node val)
{
	vect->size++;
	vect->data = realloc(vect->data, vect->size*sizeof(node));
	vect->data[vect->size-1] = val; 
}

void delete_vector_node(vector_node* vect)
{
	for (int i=0; i<vect->size; i++)
	{
		delete_vector_transition(&vect->data[i].exiting);
	}
	free(vect->data);
}

typedef struct
{
	int init;
	node* hash;
	int num_elems;
	int size;
}hash_table;

hash_table input[256];
node empty_nodes[MAX_HASH];

hash_table new_hash_table()
{
	hash_table rtn;
	rtn.init = 0;
	return rtn;
}

void realloc_hash_table(hash_table* hash_t)
{	
	int new_size = hash_t->size*2;
	node* new_hash = calloc(new_size, sizeof(node));
	for (int i=0; i<hash_t->size; i++)
	{
		int state = hash_t->hash[i].state;
		if (state != 0)
		{
			int hashval = state & (new_size-1);
			while (new_hash[hashval].state != 0)
			{
				hashval++;
				if (hashval == new_size)
					hashval = 0;
			}
			new_hash[hashval].state = state;
			new_hash[hashval].exiting = hash_t->hash[i].exiting;
		}
	}
	free(hash_t->hash);
	hash_t->hash = new_hash;
	hash_t->size = new_size;
}

vector_transition* hash_table_find_or_append(hash_table* hash_t, int state)
{
	if (hash_t->init == 0)
	{
		hash_t->hash = calloc(MAX_HASH, sizeof(node));
		hash_t->size = MAX_HASH;
		hash_t->num_elems = 0;
		hash_t->init = 1;
	}
	if (hash_t->num_elems >= hash_t->size/2)
	{
		realloc_hash_table(hash_t);
	}
	int hashval = state & (hash_t->size-1);
	int i=hashval;
	do
	{
		if (hash_t->hash[i].state == state)
			return &hash_t->hash[i].exiting;
		else if (hash_t->hash[i].state == 0)
		{
			hash_t->hash[i].state = state;
			hash_t->hash[i].exiting = new_vector_transition();
			hash_t->num_elems++;
			return &hash_t->hash[i].exiting; 
		}
		i++;
		if (i == hash_t->size)
			i=0;
	} while (i!=hashval);
	abort(); //Something went VERY wrong.
}

vector_transition* hash_table_find(hash_table* hash_t, int state)
{
	int hashval = state & (hash_t->size-1);
	int i=hashval;
	do
	{
		if (hash_t->hash[i].state == state)
			return &hash_t->hash[i].exiting;
		else if (hash_t->hash[i].state == 0)
			return NULL;
		i++;
		if (i == hash_t->size)
			i=0;
	} while (i!=hashval);
	return NULL;
}

void complete_hash_tables()
{
	for (int i=0; i<256; i++)
	{
		if (input[i].init == 0)
		{
			input[i].hash = empty_nodes;
		}
	}
}

///#######

typedef struct
{
	char* data;
	int size;
	int len;
	int count;
}cow_string;

cow_string* original_ribbon;

cow_string* new_cow_string_from_string(char* stringa)
{
	cow_string* rtn = malloc(sizeof(cow_string));
	rtn->data = stringa;
	rtn->size = rtn->len = strlen(stringa);
	rtn->count=1;
	return rtn;
}

cow_string* new_cow_string()
{
	cow_string* rtn = malloc(sizeof(cow_string));
	rtn->data = malloc(BASIC_ALLOC);
	rtn->len = 0;
	rtn->size = BASIC_ALLOC;
	rtn->count=1;
	return rtn;
}

cow_string* new_cow_string_from_cow_string(cow_string* cow)
{
	cow_string* copy = malloc(sizeof(cow_string));
	copy->data = malloc(cow->size);
	copy->len = cow->len;
	copy->size = cow->size;
	copy->count = 1;
	memcpy(copy->data, cow->data, copy->len);
	return copy;
}

char access_cow_string(cow_string* stringa, int pos)
{
	if (pos < stringa->len)
	{
		return stringa->data[pos];
	}
	else
	{
		return '_';
	}
}

cow_string* modify_cow_string(cow_string* stringa, int pos, char c)
{
	cow_string* rtn;
	if (pos < stringa->size)
	{
		if (pos < stringa->len && stringa->data[pos] == c)
			return stringa;
		if (stringa->count > 1)
		{
			rtn = new_cow_string_from_cow_string(stringa);
			stringa->count--;
			stringa = rtn;
		}
		stringa->data[pos] = c;
		if (pos == stringa->len)
			stringa->len++;
		return stringa;
	}
	else
	{
		if (stringa->count > 1)
		{
			rtn = new_cow_string_from_cow_string(stringa);
			stringa->count--;
			stringa = rtn;
		}
		
		stringa->size*=16;
		stringa->data = realloc(stringa->data, stringa->size);
		
		stringa->data[pos] = c;
		++stringa->len;
		return stringa;
	}
}

void delete_cow_string(cow_string* cow)
{
	cow->count--;
	if (cow->count == 0)
	{
		free(cow->data);
		free(cow);
	}
}

typedef struct
{
	cow_string* ribbon_left;
	cow_string* ribbon_right;
	int headpos;
	int state;
	long int ttl;
}status;

status* new_status()
{
	status* rtn = malloc(sizeof(status));
	rtn->headpos = 0;
	rtn->ttl = max;
	rtn->state = 1;
	rtn->ribbon_right = new_cow_string_from_cow_string(original_ribbon);
	rtn->ribbon_left = new_cow_string();
	return rtn;
}

status* new_status_from_status(status* s)
{
	status* rtn = malloc(sizeof(status));
	rtn->ribbon_left = s->ribbon_left;
	rtn->ribbon_right = s->ribbon_right;
	s->ribbon_left->count++;
	s->ribbon_right->count++;
	rtn->headpos = s->headpos;
	rtn->ttl = s->ttl;
	rtn->state = s->state;
	return rtn;	
}

char read_char_status(status* src)
{
	if (src->headpos >= 0)
		return access_cow_string(src->ribbon_right, src->headpos);
	else
		return access_cow_string(src->ribbon_left, -(src->headpos+1));
}

void change_status(status* s, char c, char dir, int final)
{
	s->state = final;
	--s->ttl;
	if (s->headpos >= 0)
		s->ribbon_right = modify_cow_string(s->ribbon_right, s->headpos, c);
	else
		s->ribbon_left = modify_cow_string(s->ribbon_left, -(s->headpos+1), c);
	s->headpos = s->headpos + dir;
}

void delete_status(status* s)
{
	delete_cow_string(s->ribbon_left);
	delete_cow_string(s->ribbon_right);
	free(s);
}

typedef struct list_node_
{
	struct list_node_* prev;
	struct list_node_* next;
	status* state;
}list_node_t;

typedef struct
{
	list_node_t* head;
	list_node_t* tail;
	list_node_t* reserve;
}list_status;

list_status* new_list_status()
{
	list_status* list = malloc(sizeof(list_status));
	list->head = list->tail = list->reserve = NULL;
	return list;
}

void append_list(list_status* list, status* state)
{
	if (list->head == NULL) //Devo riempire
	{
		if (list->reserve != NULL)
		{
			list->head = list->reserve;
			list->tail = list->reserve;
			list->head->state = state;
			list->reserve = list->reserve->next;
		}
		else
		{
			list->head = list->tail = malloc(sizeof(list_node_t));
			list->head->prev = list->head->next = NULL;
			list->head->state = state;
			list->tail = list->head;
		}
	}
	else
	{
		if (list->reserve != NULL) //Tail ha successore, inserisco lì
		{
			assert(list->tail->next == list->reserve);
			list->tail = list->reserve;
			list->tail->state = state;
			list->reserve = list->reserve->next;
		}
		else
		{
			list_node_t* newnode = malloc(sizeof(list_node_t));
			newnode->state = state;
			newnode->prev = list->tail;
			newnode->prev->next = newnode;
			newnode->next = NULL;
			list->tail = newnode;
		}
	}
}

void prepend_list(list_status* list, status* state)
{
	if (list->head == NULL) //Devo riempire
	{
		list->head = list->tail = malloc(sizeof(list_node_t));
		list->head->prev = list->head->next = NULL;
		list->head->state = state;
		list->tail = list->head;
	}
	else
	{
		list_node_t* newnode = malloc(sizeof(list_node_t));
		newnode->state = state;
		newnode->next = list->head;
		newnode->next->prev = newnode;
		newnode->prev = NULL;
		list->head = newnode;
	}
}

status* pop_list_front(list_status* list) //CFS
{
	status* rtn;
	if (list->head->next != NULL)
		list->head->next->prev = NULL;
	rtn = list->head->state;
	list_node_t* temp = list->head;
	list->head = list->head->next;
	free(temp);
	return rtn;
}

status* pop_list_back(list_status* list) //DFS
{
	status* rtn;
	if (list->tail == list->head)
	{
		rtn = list->tail->state;
		assert(list->reserve == list->tail->next);
		list->reserve = list->tail;
		list->tail = list->head = NULL;
		return rtn;
	}
	rtn = list->tail->state;
	assert(list->reserve == list->tail->next);
	list->reserve = list->tail;	
	list->tail = list->tail->prev;	
	return rtn;
}

void clear_list(list_status* list)
{
	list_node_t* nodo;
	list_node_t* temp;
	nodo = list->head;
	if (nodo != NULL)
	{
		while (nodo != list->reserve)
		{
			temp = nodo;
			nodo = nodo->next;
			delete_status(temp->state);
			free(temp);
		}
	}
	list->head = list->tail = NULL;
}

void list_dump_pool(list_status* list)
{
	list_node_t* nodo;
	list_node_t* temp;
	nodo = list->reserve;
	while (nodo != NULL)
	{
		temp = nodo;
		nodo = nodo->next;
		free(temp);
	}
}

char is_empty_list(list_status* list)
{
	return list->head == NULL;
}

int main()
{
	int dump;
	
	char* stringa=malloc(100);
	dump = scanf("tr \n");
	dump = scanf("%s", stringa);
	
	while (stringa[0] != 'a')
	{
		int init;
		char read;
		init = atoi(stringa)+1;
		dump = scanf(" %c %c %c", &read, &app_trans_write, &app_trans_mov); 
		dump = scanf("%d",  &app_trans_final);
		dump = scanf("%s", stringa);
		vector_transition* trans = hash_table_find_or_append(&input[(int)read], init);
		append_transition(trans);
	}
	
	complete_hash_tables();
		
	dump = scanf("%s", stringa);
	while (stringa[0] != 'm')
	{
		int acc = atoi(stringa)+1;
		set_bit_n(acc);
		dump = scanf("%s", stringa);
	}
	
	dump = scanf("%ld", &max);
	dump = scanf("%*s"); //run

	list_status* lista;
	lista = new_list_status();
	
	free(stringa);
	
	dump = scanf("%ms", &stringa);
	
	while (dump != EOF)
	{
		original_ribbon = new_cow_string_from_string(stringa);
		append_list(lista, new_status());
		int finish=0, unterminated=0;
		while (!is_empty_list(lista))
		{
			finish = 0;
			status* actual_status;
			if (max < 1000000)
				actual_status = pop_list_front(lista);
			else
				actual_status = pop_list_back(lista);
			while (1)
			{
				char actual_char = read_char_status(actual_status);
				int state = actual_status->state;
				
				vector_transition* trans = hash_table_find(input+actual_char, state);
				if (trans == NULL)
				{
					finish = retrieve_bit_n(state);
					delete_status(actual_status);
					break;
				}
				if (actual_status->ttl == 0)
				{
					delete_status(actual_status);
					unterminated = 1;
					break;
				}
			
				int j;
				int numElems = trans->size;

				for (j=0; j<numElems-1; j++)
				{
					status* tempstat = new_status_from_status(actual_status);
					change_status(tempstat, trans->data[j].write, trans->data[j].mov, trans->data[j].final);
					append_list(lista, tempstat);
				}
				change_status(actual_status, trans->data[j].write, trans->data[j].mov, trans->data[j].final);
			}
			if (finish == 1)
			{
				printf("1\n");
				break;
			}
		}
		if (finish == 0)
		{
			if (unterminated == 1)
				printf("U\n");
			else
				printf("0\n");
		}
		clear_list(lista);
		delete_cow_string(original_ribbon);
		dump = scanf("%ms", &stringa);
	}
	list_dump_pool(lista);
	free(lista);
	free(stringa);
}
