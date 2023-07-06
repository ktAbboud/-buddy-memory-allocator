//
// Created by student on 7/1/23.
//
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <cstdint>


#define MAXSIZE 100000000
#define MAX_ORDER 11
#define MMAP_SIZE 128*1024
#define BASE 128
#define MINSIZE 128
#define FAIL -1
#define BLOCKS_NUMBER 32


bool start=false;
size_t allocated_and_used_bytes=0;
size_t allocated_and_used_blocks=0;
int COOKIE;

int pow_of_2(int num){
    if (num <0){
        return FAIL;
    }

    int x=1;
    while (num > 0){
        x= x*2;
        num--;
    }
    return x;
}


int list_index(size_t size){
    for(int i=0; i< MAX_ORDER; i++){
        if (BASE * pow_of_2(i) -size == 0){
            return i;
        }
    }
    return FAIL; // won't get here
}


typedef struct MallocMetadata {
    int start_cookie;
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
    bool is_mmap;
    void* address;
    int end_cookie;
} Metadata;

class MallocMetadataList {
public:
    Metadata *head;
    size_t size;
    size_t size_block;
    MallocMetadataList();
    void add(Metadata* md);
    void remove(Metadata* md);
};

MallocMetadataList::MallocMetadataList() {
    head= NULL;
    size=0;
}

class Meta_data_arr{
public:
    Metadata * array[MAX_ORDER];
    Meta_data_arr();
};


Meta_data_arr ::Meta_data_arr(){
    for(int i=0; i <MAX_ORDER;i++){
        array[i]=NULL;
    }
}


Meta_data_arr object;
Metadata **arr=object.array;



void check_cookies(Metadata* md){
    if (md->start_cookie!= md->end_cookie){
        exit(0xdeadbeef);
    }
    return;
}


Metadata * remove_head(Metadata** arr,int i){
    if(arr[i]==NULL){
        return NULL;
    }
    Metadata* old_head= arr[i];
    check_cookies(old_head);
    Metadata* new_head= old_head->next;
    arr[i]=new_head;
    if(new_head!=NULL){
        check_cookies(new_head);
        new_head->prev = NULL;
    }
    old_head->next=NULL;
    old_head->prev=NULL;
    return old_head;
}



void init_Meta_struct(Metadata * meta,size_t size, bool is_free,bool is_mmap,void* address){
    meta->size=size;
    meta->is_free=is_free;
    meta->start_cookie=COOKIE;
    meta->end_cookie=COOKIE;
    meta->next=NULL;
    meta->prev=NULL;
    meta->is_mmap=is_mmap;
    meta->address=address;

}

MallocMetadataList mmap_list=MallocMetadataList();

void MallocMetadataList::add(Metadata* md)
{
    if(md==NULL){
        return;
    }
    check_cookies(md);
    if(head==NULL)
    {
        head=md;
        size+=1;
        return;
    }
    check_cookies(head);
    if ((char*)md->address < (char*)head->address ){ // should add it as a head
        md->next=head;
        md->prev=NULL;
        head->prev=md;
        head=md;
        size+=1;
        return;
    }
    Metadata* tmp=head;
    while ((char*)tmp->address <(char*) md->address){
        check_cookies(tmp);
        if (tmp->next == NULL){ // add it as the last element
            // md is after tmp
            md->prev=tmp;
            tmp->next=md;
            md->next=NULL;
            size+=1;
            return;
        }
        tmp=tmp->next;  // tmp here is never NULL
        check_cookies(tmp);
    }
    // md should be before tmp
    md->prev=tmp->prev;
    md ->prev->next=md;
    md->next=tmp;
    tmp->prev= md;
    size+=1;
    return;
}

size_t size_of_list(Metadata* head){
    Metadata* tmp= head;
    size_t num=0;
    while(tmp!=NULL){
        num++;
        check_cookies(tmp);
        tmp=tmp->next;
    }
    return num;
}

void add_to_list(Metadata* head,Metadata* md)
{
    if(md==NULL){
        return;
    }
    check_cookies(md);
    int index= list_index(md->size);
    if(head==NULL)
    {
        arr[index]=md;
        md->next=NULL;
        md->prev=NULL;
        return;
    }
    check_cookies(head);
    if ((char*)md->address < (char*)head->address ){ // should add md as head
        md->next=head;
        md->prev=NULL;
        head->prev=md;
        arr[index]=md;
        return;
    }
    Metadata* tmp=head;
    while ((char*)tmp->address <(char*) md->address){
        if (tmp->next == NULL){ // add it as the last element
            // md is after tmp

            md->prev=tmp;
            tmp->next=md;
            md->next=NULL;
            return;
        }
        tmp=tmp->next; // never gets here if tmp->next is NULL
        check_cookies(tmp);
    }
    // md should be before tmp
    md->prev=tmp->prev;
    md ->prev->next=md;
    md->next=tmp;
    tmp->prev= md;
    return;
}

void MallocMetadataList::remove(Metadata* md)
{
    if (md==NULL){
        return;
    }
    check_cookies(md);
    if(head==NULL){
        return;
    }
    check_cookies(head);
    if (head==md){ //we want to delete the first element
        Metadata* tmp=head->next;
        if(tmp!=NULL){
            check_cookies(tmp);
            tmp->prev=NULL;
        }
        head=tmp;
        size -=1;
        md->next=NULL;
        md->prev=NULL;
        return;
    }
    Metadata* tmp=head;
    for(size_t i=0;i<size;i++)
    {
        check_cookies(tmp);
        if(tmp==md)
        {
            if(tmp->prev!=NULL)
            {
                tmp->prev->next=tmp->next;
                tmp->prev=NULL;
            }
            if(tmp->next!=NULL)
            {
                tmp->next->prev=tmp->prev;
                tmp->next=NULL;
            }
            size -=1;
            return;
        }
        tmp=tmp->next;
    }
}


void remove_from_list(Metadata *head,Metadata* md)
{
    if (md==NULL){

        return;
    }
    check_cookies(md);
    if(head==NULL){
        return;
    }
    check_cookies(head);
    if (head==md){ //we want to delete the first element
        Metadata* tmp=head->next;
        if(tmp!=NULL){
            check_cookies(tmp);
            tmp->prev=NULL;
        }
        int index= list_index(md->size);
        md->next=NULL;
        md->prev=NULL;
        arr[index]=tmp;
        return;
    }

    Metadata* tmp=head;
    while(tmp!=NULL)
    {
        check_cookies(tmp);
        if(tmp==md)
        {

            if(tmp->prev!=NULL)
            {
                tmp->prev->next=tmp->next;
                tmp->prev=NULL;
            }
            if(tmp->next!=NULL)
            {
                tmp->next->prev=tmp->prev;
                tmp->next=NULL;
            }
            return;
        }
        tmp=tmp->next;
    }
}

void seperate_block(Metadata* meta,Metadata** arr, int index){ // check size of block
    if(meta==NULL){
        return;
    }
    check_cookies(meta);
    if(meta->size <= MINSIZE){
        return;
    }
    meta->size=meta->size/2; // divide into half
    size_t size=meta->size;
    meta->next=NULL;
    meta->prev=NULL;
    void* addr=(void*)((char*)meta->address+size);
    Metadata* second_meta=(Metadata*)addr;
    init_Meta_struct(second_meta,meta->size,true, false,(void*)((char*)(meta->address)+meta->size));
    add_to_list(arr[index],meta);
    add_to_list(arr[index],second_meta);

}

Metadata* find_suitable_block(size_t size)
{
    Metadata* block_to_return;
    for(int i=0;i<MAX_ORDER;i++)
    {
        if(size_of_list(arr[i])!=0)
        {
            check_cookies(arr[i]);
            if( (arr[i]->size>size)&&((( (arr[i]->size)/2 -sizeof (Metadata) )>=size)) && arr[i]->size > MINSIZE) // it can fit into a smaller block of power 2
            {
                Metadata* meta=remove_head(arr,i);
                seperate_block(meta,arr,i-1);
                return find_suitable_block(size);
            }
            else if(arr[i]->size -sizeof(Metadata) >= size){
                block_to_return= remove_head(arr,i);
                return block_to_return;
            }
            else {
                return NULL;
            }
        }
    }
    return NULL;
}

bool buddies(Metadata* first, Metadata* second)
{
    if (first== NULL || second==NULL) {
        return false;
    }
    check_cookies(first);
    check_cookies(second);
    if((((long)first->address ^ (long)first->size))== (long)second->address)
    {
        return true;
    }
    if((((size_t )second->address ^ second->size))== (size_t)first->address){
        return true;
    }
    return false;
}

void Merge_blocks(Metadata** arr, int i ,Metadata* first_md,Metadata* second_md)
{
    if(first_md == NULL || second_md == NULL){
        return;
    }
    check_cookies(first_md);
    check_cookies(second_md);
    remove_from_list(arr[i],first_md);
    remove_from_list(arr[i],second_md);
    first_md->size=first_md->size+second_md->size;
    second_md->size =second_md->size+ first_md->size;
    first_md->next=NULL;
    first_md->prev=NULL;
    second_md->next=NULL;
    second_md->prev=NULL;
    if ((char*)second_md->address < (char*)first_md -> address){
        add_to_list(arr[i+1],second_md);

    }
    else {
        add_to_list(arr[i+1],first_md);


    }
}

void Merge()
{
    // always go back to head and check
    for(int i=0; i< MAX_ORDER-1; i++){
        Metadata* tmp=arr[i];
        while (tmp !=NULL && tmp->next != NULL){
            check_cookies(tmp);
            Metadata* first= tmp;
            Metadata* second= tmp->next;
            check_cookies(second);

            if (buddies(first,second)){
                Merge_blocks(arr,i,first,second);
                size_of_list(arr[i]) ;
                tmp=arr[i];
            }
            else
            {
                tmp=tmp->next;
            }
        }
    }
}


void init_malloc()
{

    srand(time(NULL));
    COOKIE=(int)std::rand(); // Generate a random number between 0 and RAND_MAX
    unsigned long address=(unsigned long) sbrk(0); // get address
    intptr_t add_alignment=(((intptr_t ) address/(BLOCKS_NUMBER*MMAP_SIZE))+1)*(BLOCKS_NUMBER*MMAP_SIZE);
    void* alignment = sbrk(add_alignment - address);
    if( alignment== ((void*) -1 )){
        return;
    }
    void* addr=sbrk(MMAP_SIZE * BLOCKS_NUMBER );
    if( addr== ((void*) -1 )){
        return;
    }
    for(int i=0;i<BLOCKS_NUMBER; i++){
        Metadata* meta=(Metadata*)addr;
        init_Meta_struct(meta,MMAP_SIZE,true,false,addr);;
        add_to_list(arr[MAX_ORDER-1],meta);
        addr = (void*)(( char*)addr + MMAP_SIZE);
    }
    return ;
}

void* smalloc(size_t size)
{
    if (start==false){
        init_malloc();
        allocated_and_used_blocks=0;
        allocated_and_used_bytes=0;
        start=true;
    }
    if(size==0)
    {
        return NULL;
    }
    if(size>MAXSIZE)
    {
        return NULL;
    }

    if(size>MMAP_SIZE)
    {
        void* addr= mmap(NULL,size+sizeof(Metadata), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
        if(addr == (void*) -1){
            return NULL;
        }
        Metadata* mmap_block= (Metadata*) addr;
        init_Meta_struct(mmap_block,size,false,true,addr);
        mmap_list.add(mmap_block);
        check_cookies(mmap_block);
        allocated_and_used_bytes+= mmap_block->size;
        allocated_and_used_blocks+=1;
        return (void*)((char*)(mmap_block->address) + sizeof(Metadata));
    }
    Metadata* block = find_suitable_block(size);
    if (block == NULL){
        return NULL;
    }
    else{ // we found a block
        check_cookies(block);
        block->is_free=false;
        block->next=NULL;
        block->prev=NULL;
        allocated_and_used_bytes=  allocated_and_used_bytes+ block->size -sizeof (Metadata);
        allocated_and_used_blocks+=1;
        //already deleted it from list in function find_suitable_block
        return (void*)(block+1);
    }
    return NULL;
}

void* scalloc(size_t num, size_t size)
{
    size_t total_size=num*size;
    void* addr=smalloc(total_size);
    if(addr== NULL)
    {
        return NULL;
    }
    memset(addr,0,total_size);
    return addr;
}


void sfree(void* p)
{
    if(p== NULL)
    {
        return;
    }

    Metadata* mmd=(Metadata*)((char*)p-sizeof(Metadata));
    if(mmd==NULL){
        return;
    }
    check_cookies(mmd);
    if(mmd->is_free){
        return;
    }
    if(mmd->is_mmap){
        allocated_and_used_bytes-= mmd->size;
        allocated_and_used_blocks-=1;
        mmd->is_free=true;
        mmap_list.remove(mmd);
        munmap((void*)((char*)p-sizeof(Metadata)),mmd->size+sizeof (Metadata));
        return;
    }
    mmd->is_free=true;
    mmd->next=NULL;
    mmd->prev=NULL;
    // add to list
    int index = list_index(mmd->size);
    add_to_list(arr[index],mmd);
    allocated_and_used_bytes=allocated_and_used_bytes- mmd->size + sizeof (Metadata);
    allocated_and_used_blocks-=1;
    Merge();
}




bool check_merge_possibility(Metadata * meta,size_t size)
{
    int i= list_index(meta->size);
    if (i >=MAX_ORDER-1){
        return false;
    }
    if(meta==NULL){
        return false;
    }
    bool can_merge=false;
    Metadata* tmp=arr[i];
    while (tmp !=NULL){
        check_cookies(tmp);
        if (buddies(tmp,meta)){
            Merge_blocks(arr,i,meta, tmp);
            if (tmp->address < meta->address){
                meta -> address= tmp->address;
            }
            if (meta->size - sizeof (Metadata)>= size){
                return true; // we can merge !
            }

            can_merge = check_merge_possibility(meta,size);
            if(can_merge){
                return true;
            }
            else{
                seperate_block(meta,arr,i);
            }
        }
        tmp=tmp->next;
    }
    return false;
}


void* srealloc(void* oldp, size_t size){
    if (size ==0 || size>MAXSIZE){
        return NULL;
    }
    if (oldp == NULL) {
        return smalloc(size);
    }
    Metadata* mmd = (Metadata*)((char*)(oldp) - sizeof(Metadata));
    check_cookies(mmd);
    if (mmd->is_mmap){
        if(mmd->size == size){
            return oldp;
        }
        else {
            allocated_and_used_bytes=allocated_and_used_bytes- mmd->size;
            allocated_and_used_blocks-=1;
            mmap_list.remove(mmd);
            munmap((void*)((char*)oldp-sizeof(Metadata)),mmd->size+sizeof(Metadata));
            void* addr= mmap(NULL,size+sizeof(Metadata), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
            Metadata* mmap_block= (Metadata*) addr;
            init_Meta_struct(mmap_block,size,false,true,addr);
            mmap_block=(Metadata*) addr;
            mmap_list.add(mmap_block);
            check_cookies(mmap_block);
            allocated_and_used_bytes+= mmap_block->size;
            allocated_and_used_blocks+=1;
            return (void*)((char*)(mmap_block->address) + sizeof(Metadata));
        }
    }
    if(mmd->size - sizeof (Metadata) >= size){ // case a
        return oldp ;
    }
    else {
        void* dest;
        // case b
        void * curr_addr= mmd->address;
        size_t curr_size = mmd->size;

        bool merge=check_merge_possibility(mmd,size);
        for(int x=0;x<11;x++)
        remove_from_list(arr[list_index(mmd->size)],mmd);
        if(merge){
            mmd->is_free=false;
            mmd->next=NULL;
            mmd->prev=NULL;
            allocated_and_used_bytes=  allocated_and_used_bytes - curr_size + sizeof(Metadata) ;
            allocated_and_used_bytes= allocated_and_used_bytes + mmd->size - sizeof (Metadata );
            return (void*) (mmd+1);
        }
        mmd->address=curr_addr;
        mmd->size = curr_size;
        // case c
        if( (dest=smalloc(size)) == NULL) { // sbrk() failed
            return NULL;
        }
        memmove( dest, oldp,size); //copy <size> bytes from oldp to dest
        mmd->is_free=true; // free oldp block // add to list
        int index = list_index(mmd->size);
        add_to_list(arr[index],mmd);
        allocated_and_used_bytes=allocated_and_used_bytes - mmd->size;
        allocated_and_used_blocks-=1;
        Merge();
        return dest;

    }
    return NULL; // shouldn't get here
}

size_t _num_free_blocks() //5
{
    size_t num_free=0;
    for(size_t i=0;i<MAX_ORDER;i++)
    {
        num_free+= size_of_list(arr[i]);
    }
    return num_free;
}

size_t _num_free_bytes() //6
{
    size_t num_free=0;
    for(size_t i=0;i< MAX_ORDER;i++)
    {
        if(arr[i] !=NULL){
            check_cookies(arr[i]);
            num_free+=( size_of_list(arr[i]))*(arr[i]->size);
        }
    }
    return (num_free - _num_free_blocks()*sizeof (Metadata) );
}

size_t _num_allocated_blocks() //7
{
    return (_num_free_blocks() + allocated_and_used_blocks );
}

size_t _num_allocated_bytes() //8
{

    return ( _num_free_bytes() + allocated_and_used_bytes );
}

size_t _num_meta_data_bytes() //9
{
    return _num_allocated_blocks() * sizeof (Metadata);
}

size_t _size_meta_data() //10
{
    return sizeof (Metadata);
}