#define G_VC
#include "xerxes.h"
#include "vccode.h"

/*******************************/

#define s8  char
#define u8  unsigned char
#define s16 short
#define u16 unsigned short
#define s32 int
#define u32 unsigned int

/******** aen's RAWMEM class ********/

class rangenum
{
	s32 lo,hi;
	s32 n;

public:
	rangenum(s32 l=0, s32 h=0)
	{
		set_limits(l,h);
		set(0);
	}

	void set_limits(s32 l, s32 h)
	{
		lo=l;
		hi=h;
	}
	void set(s32 x)
	{
		if (x<lo || x>hi)
			return;
		n=x;
	}
	void inc(s32 x)
	{
		set(get()+x);
	}
	s32 get() const { return n; }
	s32 get_lo() const { return lo; }
	s32 get_hi() const { return hi; }
};

#define HUNK 32
#define HUNK_COUNT(x) (((x)+HUNK-1)/HUNK)
#define RAWMEM_USAGE_STRING_LENGTH 256

extern void die_on_violation(u32 flag);

class rawptr;
class rawmem
{
	// what are we using this memory for?
	char m_use[RAWMEM_USAGE_STRING_LENGTH +1];

	u8* m_data;			// the raw buffer
	u32 m_length;		// user size request for buffer; mostly for suffix corruption detection
	u32 m_hunks;		// hunks currently allocated
	u32 m_touched;		// byte total of memory in-use

	// used by ctors
	void zero_all();
	// total amount of memory this rawmem object currently contains
	u32 bytes_allocated() const;

public:
	rawmem(s32 requested=0, const char *use=0L);
	~rawmem();

	u32 hunks() const;
	u32 touched() const;
	void touch(u32 rhand);
	void untouch();
	u32 length() const;

	void destroy();
	void resize(s32 requested, const char *use=0L);
	void become_string(const char* text);

	// reallocate if hunks required to hold length() bytes is less than
	// the currently allocated number of blocks
	void consolidate();
	void report();

	u8* get(s32 n, u32 z) const;
	u32* get_u32(s32 n) const;
	u16* get_u16(s32 n) const;
	u8* get_u8(s32 n) const;
	u8& operator[](s32 n) const;

	void set_use(const char *use);
	char* get_use() const;

	friend void rawmem_fill_u8(rawptr& dest, u32, s32);
	friend void rawmem_xfer(rawptr& dest, rawptr& source, s32);
};

class rawptr {
	rawmem *raw;
	rangenum offset;
public:
	rawptr(){}
	rawptr(rawmem* r, u32 pos=0) {
		point_to(r);
		set_pos(pos);
	}

	void touch(u32 count) {
		if (!raw) return;
		raw->touch(get_pos()+count);
	}

	void point_to(rawmem* r) {
		if (!r) return;
		raw=r;
		offset.set_limits(0,r->length()-1);
	}
	//rawmem* pointing_to() const { return raw; }

	u8 *get(u32 count) {
		if (!raw) return 0L;
		return raw->get(get_pos(),count);
	}

	void set_pos(u32 x) {
		offset.set(x);
	}
	s32 get_pos() const { return offset.get(); }
	void inc_pos(s32 x) { offset.inc(x); }

	void set_start() {
		set_pos(0);
	}
	void set_end() {
		if (!raw) return;
		set_pos(raw->length()-1);
	}
};

static u32 die_on_violate=1;
void die_on_violation(u32 flag) {
	die_on_violate=flag;
}

// ctor
rawmem::rawmem(s32 requested, const char *use) {
	zero_all();
	resize(requested,use);
}
// dtor
rawmem::~rawmem(){
	destroy();
}

u32 rawmem::touched() const {
	return m_touched;
}
void rawmem::touch(u32 rhand) {
	if (rhand>length())
		rhand=length();
	if (rhand>touched())
		m_touched = rhand;
}

void rawmem::untouch() {
	m_touched=0;
}

// used by ctors
void rawmem::zero_all() {
	m_data=0L;
	m_length=m_hunks=m_touched=0;
}
char* rawmem::get_use() const {
	static char temp[RAWMEM_USAGE_STRING_LENGTH +1];
	if (strlen((char *)m_use)<1)
		strcpy(temp, "*UNKNOWN*");
	else strcpy(temp, (char *)m_use);
	return temp;
}

void rawmem::set_use(const char *use) {
	if (!use || strlen((char *)use)<1) return;
	else strncpy((char *)m_use, (char *)use, 256);
}

void rawmem::become_string(const char* text) {
// resize to accomodate a C-string (zero-terminated), then copy the string in and touch all
	resize(strlen((char *)text)+1);
	memcpy(get(0,length()), (char *)text, length());
	touch(length());
}

u32 rawmem::length() const { return m_length; }
u32 rawmem::hunks() const { return m_hunks; }
// total amount of memory this rawmem object currently contains
u32 rawmem::bytes_allocated() const { return hunks() * HUNK; }

// free all memory and zero all members; also effectively unmaps a device, if currently mapped
void rawmem::destroy() {
	if (m_data) {
		delete [] m_data;
		m_data = 0L;
	}
	m_length=m_hunks=m_touched=0;
}

// aen <21dec99> Allocation management core.
void rawmem::resize(s32 requested, const char *use) {
	set_use(use);
	//if (requested<1)
	//	requested=1;
	// do we need more blocks?
	if (requested && HUNK_COUNT(requested) > (signed) hunks()) {
		m_hunks = HUNK_COUNT(requested);			// reset hunk count
		u8* buf = new u8 [bytes_allocated()];	// allocate a new buffer w/ more needed hunks
		// preservation requested? if so, copy in touched bytes
		if (touched()) {
			memcpy(buf, m_data, touched());
		}
		// destroy current buffer, and reassign pointer to new buffer
		delete [] m_data;
		m_data = buf;
	// forgot how this works..
	} else if (requested < (signed) length()) {
		touch(requested);
	}

	// zero-out all unused bytes during this resize
	if (touched() < bytes_allocated()) {
		memset(m_data+touched(), 0, bytes_allocated()-touched());
	}
	// reset length
	m_length=requested;
}

// reallocate if hunks required to hold length() bytes is less than
// the currently allocated number of blocks
void rawmem::consolidate() {
	// do we really need this many hunks?
	if (HUNK_COUNT(length()) < hunks()) {
		m_hunks = HUNK_COUNT(length());				// reset hunk count
		u8* buf = new u8 [bytes_allocated()];	// allocate a new buffer w/ less hunks
		// any memory in-use? if so, copy it in
		if (touched()) {
			memcpy(buf, m_data, touched());
		}
		// destroy current buffer, and reassign pointer to new buffer
		delete [] m_data;
		m_data = buf;
	}
}

void rawmem::report() {
	printf("$ MEM REPORT <%s> # ", get_use());
	printf("<L%d:%d%%> <H%d> <T%d:%d%%>\n",
		length(),
		(length()*100)/bytes_allocated(),
		hunks(),
		touched(),
		(touched()*100)/length()
	);
}

// aen <9nov99> : Yeah baby! The core of all protection.
u8* rawmem::get(s32 n, u32 z) const {
	// all protection for all requests originates here
	if (n<0) {
		if (die_on_violate) {
			n=0-n;
			printf("rawmem$get: %s: prefix violation (offset=%d,count=%d)\n", get_use(), n, z);
			exit(-1);
		}
		n=0;
	// we want things to be okay if length (z) is zero; if we don't check for length,
	// it will trigger the violation code, even though a violation hasn't occurred.
	} else if (z && n+z-1 >= length()) {
		if (die_on_violate) {
			n=n+z-1 -length() +1;
			printf("rawmem$get: %s: suffix violation (offset=%d,count=%d)\n", get_use(), n, z);
			exit(-1);
		}
		n=0;
	}
	return &m_data[n];
}

u32* rawmem::get_u32(s32 n) const { return (u32*)get(n, sizeof(u32)); }
u16* rawmem::get_u16(s32 n) const { return (u16*)get(n, sizeof(u16)); }
u8* rawmem::get_u8(s32 n) const { return get(n, sizeof(u8)); }
u8& rawmem::operator [](s32 n) const { return *get(n, sizeof(u8)); }

void rawmem_fill_u8(rawptr& dest, u32 color, s32 count) {
	if (count<0) return;
	u8* d=dest.get(count);
	if (d)
		memset(d,color,count);
	dest.touch(count);
}

void rawmem_xfer(rawptr& dest, rawptr& source, s32 count) {
	if (count<0) return;
	u8 *s=source.get(count);
	u8 *d=dest.get(count);
	if (s && d)
		memcpy(d,s,count);
	source.touch(count);
	dest.touch(count);
}

#undef HUNK

/*********** aen's linked-list class ***********/

class linked_node
{
	linked_node* nex;
	linked_node* pre;

public:
	linked_node* next() { return nex; }
	linked_node* prev() { return pre; }
	void set_next(linked_node* pn) { nex=pn; }
	void set_prev(linked_node* pn) { pre=pn; }

	virtual int compare(void* c) { c=c; return 0; }

	virtual ~linked_node() {}
	linked_node() { nex=pre=0; }
};

class linked_list
{
	linked_node* fn;
	linked_node* cn;
	int nn;

public:
	~linked_list();
	linked_list(linked_node *first=0);

	void insert_head(linked_node* pn);
	void insert_tail(linked_node* pn);
	void insert_before_current(linked_node* pn);
	void insert_after_current(linked_node* pn);
	void insert(linked_node* pn);

	linked_node* current() const { return cn; }
	linked_node* head() const { return fn; }
	linked_node* tail() const { return head()->prev(); }

	linked_node* get_node(int x);

	void set_current(linked_node* pn) { cn=pn; }

	void go_head() { cn=head(); }
	void go_tail() { cn=head()->prev(); }
	void go_next() { cn=current()->next(); }
	void go_prev() { cn=current()->prev(); }

	int number_nodes() const { return nn; }
	int node_number(linked_node* pn);

	int unlink(linked_node* pn);
};

void linked_list::insert_before_current(linked_node* pn)
{
	pn->set_prev(current()->prev());
	pn->set_next(current());

	current()->prev()->set_next(pn);
	current()->set_prev(pn);

	++nn;
}

void linked_list::insert_after_current(linked_node* pn)
{
	go_next();
	insert_before_current(pn);
	go_prev();
}

// unlink take a node out of the linked list, but does not dispose of the memory
int linked_list::unlink(linked_node* pn)
{
	if (!head()) return 0;

	// if they want to unlink the first node
	if (head() == pn)
	{
		head()->prev()->set_next(head()->next());	// set the first's last's next to the first's next
		head()->next()->set_prev(head()->prev());	// set the first next's last to the first last

		// if there is only one node
		if (head()->next() == head())
		{
			// clear the list
			fn=0;
			cn=0;
		}
		else
		{
			// else point the first pointer to the next node
			fn=pn->next();
		}
		// decrement the number of nodes in the list
		--nn;

		return 1;
	}
	else
	{
		// find the node in the list
		go_head();
		go_next();
		while (current() != pn && current() != head())
		{
			go_next();
		}
		// is it in the list at all?
		if (current() != head())
		{
			// yes unlink the pointers
			current()->prev()->set_next(current()->next());
			current()->next()->set_prev(current()->prev());

			// decrement the number of nodes
			--nn;

			return 1;
		}
	}

	return 0;
}

// this function clears all the nodes in a linked list and dispose of the
// memory for each one by calling is't destructor
linked_list::~linked_list()
{
	if (head())
	{
		// set the last nodes next to NULL
		// so we can go until we hit NULL
		head()->prev()->set_next(0);
	}

	// repeat until we get to that node
	while (fn != 0)
	{
		go_head();
		go_next();
		//cn=head()->next();
		delete fn;	// delete the old node
		fn=current();
	}
	// clear the list
	cn=0;
	// set the number of nodes to 0
	nn=0;
}

// this function returns the node number a node is in a linked list
// it start at the node and goes backwards until it reaches the first
// node
int linked_list::node_number(linked_node* pn)
{
	int x=1;
	while (pn != head())
	{
		x++;
		pn=pn->prev();
	}
	return x;
}

// this function returns a pointer to the xth node
linked_node* linked_list::get_node(int x)
{
	// start at the first node
	go_head();

	if (x>0 && nn) {
		x--;
		x%=nn;
		x++;
		// go forward X-1 nodes
		while (x-- > 1)
		{
			go_next();
		}
	}

	return current();
}

// this function adds a node to the end of a linked_list
void linked_list::insert_tail(linked_node* pn)
{
	// if there are no nodes, then this one becomes the first
	if (0 == head())
	{
		fn=pn;
		// and it poits to itself for first and last
		head()->set_next(head());
		head()->set_prev(head());

		++nn;
	}
	else
	{
		go_head();
		insert_before_current(pn);
	}
}

// to add a node at the fron of the list, just add it at the end and set
// the first pointer to it
void linked_list::insert_head(linked_node* pn)
{
	insert_tail(pn);
	fn=pn;
}

// insert adds a node in the list according to is sort value
void linked_list::insert(linked_node* pn)
{
	// if there are nodes, or it belongs at the beginin call add front
	if ((0==head()) || (pn->compare(head()) > 0))
	{
		insert_head(pn);
	}
	// else if it goes at the ned call add_end
	else if (pn->compare(head()->prev()) <= 0)
	{
		insert_tail(pn);
	}
	// otherwise we have to find the right spot for it.
	else
	{
		// iter starts at head
		go_head();
		while (current() != head()->prev())
		{
			go_next();
			// repeat until we find a value greater than the one we are inserting
			if (pn->compare(current()) > 0)
			{
				insert_before_current(pn);

				break;
			}
		}
	}
}

linked_list::linked_list(linked_node* first)
{
	fn=first;
	cn=first;
	nn=0;

	if (first)
	{
		linked_node* prev;

		go_head();
		do
		{
			++nn;
			prev=current();
			go_next();
		} while (current() && current() != head());

		if (0 == current())
		{
			head()->set_prev(prev);
			prev->set_next(head());
		}
		go_head();
	}
}

/*********** aen's rawstr class ************/

class string
	: public linked_node
{
	rawmem m_data;

public:
	int compare(void* n1);

	// !!! NO MEMORY PROTECTION HERE, BE CAREFUL !!!
	char* c_str() const;

	u32 length() const;

	// create & destroy
	void destroy();
	// create via C-string
	void assign(char* text);
	// create via single char
	void assign(char ch);

	// def ctor
	string(char *text = 0);
	// single character ctor
	string(char ch);
	// copy ctor
	string(const string& s);
	// dtor
	~string();

    // assignment op
	string& operator =(const string& s);
	// concatenation
	string& operator+=(const string& s);
	string& operator+ (const string& s);

	u32 touched() const { return m_data.touched(); }

	// indexed char retrieval
	char operator[](s32 n) const;

	// transformers
	string upper();
	string lower();

	// substring extraction; auto-clipping due to operator[]
	string operator()(s32 pos, s32 len); // const;
	// whee!
	string left (s32 len);
	string right(s32 len);
	string mid  (s32 pos, s32 len);

	// locators; return -1 on failure, otherwise the located index
	u32 firstindex(char c) const;
	u32 lastindex (char c) const;

	// equality ops
	bool operator ==(const string& s) const;
	bool operator < (const string& s) const;
	bool operator <=(const string& s) const;
	bool operator > (const string& s) const;
	bool operator >=(const string& s) const;

	void report();
	void consolidate();

	void become_raw(rawmem& rm);
};

int string::compare(void* c)
{
        return (*((string *)c) > *this);
}

// !!! NO MEMORY PROTECTION HERE, BE CAREFUL !!!
char* string::c_str() const {
	return (char*)m_data.get(0,m_data.length());
}

u32 string::length() const {
	return m_data.length() > 0 ? m_data.length() -1 : 0;
}

// create & destroy
void string::destroy() {
	m_data.destroy();
}
// create via C-string
void string::assign(char* text) {
	// exit gracefully, if no string
	if (!text) {
		return;
	}
	// free mem & resize
	destroy();
	// copy the string in
	m_data.become_string(text);
}
// create via single char
void string::assign(char ch) {
	char text[2] = {ch,'\0'};

	assign(text);
}

// def ctor
string::string(char *text)
: m_data(0,"string") {
	assign(text ? text : "");
}
// single character ctor
string::string(char ch)
: m_data(0,"string") {
	assign(ch);
}
// copy ctor
string::string(const string& s)
: m_data(0,"string") {
	*this = s;
}
// dtor
string::~string() {
	destroy();
}

// assignment op
string& string::operator=(const string& s) {
	if (&s != this) {
		assign((char *)s.m_data.get(0,s.m_data.length()));
	}
	return *this;
}
// concatenation
string& string::operator+=(const string& s) {
	int z = length();

	m_data.resize(z + s.length() +1);

	/*
	extern void Log(char* message);
	char shit[1024];
	sprintf(shit, "+= B4 %s", (char *)m_data.get(0,1));
	Log(shit);
	*/

	//V_memcpy(m_data.m_data+z, s.m_data, s.length());
	//m_data.m_data[z]='\0';
	rawptr source((rawmem *)&s.m_data), dest(&m_data,z);
	rawmem_xfer(dest, source, s.length());
	//(*this)[z+s.length()]='\0';
	m_data[z+s.length()]='\0';
	/*
	sprintf(shit, "+= A %s", (char *)m_data.get(0,1));
	Log(shit);
	*/

	return *this;
}
string& string::operator+(const string& s) {
	return *this += s;
}

// indexed char retrieval
char string::operator[](s32 n) const {
	return (n<0 || n>=(signed)length()) ? (char)0 : c_str()[n];
}

// transformers
string string::upper() {
	string s = *this;
	strupr((char *)s.m_data.get(0,s.m_data.length()));
	return s;
}
string string::lower() {
	string s = *this;
	strlwr((char *)s.m_data.get(0,s.m_data.length()));
	return s;
}

// substring extraction; auto-clipping due to operator[]
string string::operator()(s32 pos, s32 len) { //const {
	string s;

	// bogus length
	if (len<1 || pos>=(signed)length()) {
		return s;
	}

	// clipping
	if (pos<0) {
		len+=pos;
		pos=0;
	}
	if (pos+len>=(signed)length())
		len=length()-pos+1;

	// blap!
	s.m_data.resize(len +1);
	rawptr source(&m_data,pos), dest(&s.m_data,0);
	rawmem_xfer(dest, source, len);

	s.m_data[len]='\0';

	/*
	// slow method
	int n = 0;
	while (n < len) {
		s += operator[](pos + n);
		n++;
	}
	*/
	return s;
}
// whee!
string string::left(s32 len) { return operator()(0, len); }
string string::right(s32 len) { return operator()(length()-len, len); }
string string::mid(s32 pos, s32 len) { return operator()(pos, len); }

// locators; return -1 on failure, otherwise the located index
u32 string::firstindex(char c) const {
	/*
	char *found = strchr(c_str(), c);
	return found ? found - c_str() : -1;
	*/
	return 0;
}
u32 string::lastindex(char c) const {
	/*
	char *found = strrchr(c_str(), c);
	return found ? found - c_str() : -1;
	*/
	return 0;
}

// equality ops
bool string::operator==(const string& s) const { return strcmp(c_str(), s.c_str()) == 0; }
bool string::operator< (const string& s) const { return strcmp(c_str(), s.c_str()) <  0; }
bool string::operator<=(const string& s) const { return strcmp(c_str(), s.c_str()) <= 0; }
bool string::operator> (const string& s) const { return strcmp(c_str(), s.c_str()) >  0; }
bool string::operator>=(const string& s) const { return strcmp(c_str(), s.c_str()) >= 0; }

void string::report() {	m_data.report(); }
void string::consolidate() { m_data.consolidate(); }

void string::become_raw(rawmem& rm) {
	u32 z=strlen((char *)rm.get(0,rm.length()));
	m_data.resize(z+1);

	rawptr source(&rm), dest(&m_data);
	rawmem_xfer(dest, source, z);

	m_data[z]='\0';
}

/*******************************************/

#define USERFUNC_MARKER 10000
#define OID_VC 3

// ================================= Data ====================================

struct funcdecl
{
	char fname[40];
	char argtype[20];
	int numargs, numlocals;
	int returntype;
	int syscodeofs;
};

struct strdecl
{
	char vname[40];
	int vsofs;
	int arraylen;
};

struct vardecl
{
	char vname[40];
	int varstartofs;
	int arraylen;
};

int vc_paranoid         =0;     // paranoid VC checking

char*	sysvc			=0;
char*	mapvc			=0;
char*	basevc			=0;		// VC pool ptrs
char*	code			=0;		// VC current instruction pointer (IP)

int*	globalint		=0;		// system.vc global int variables
int		maxint			=0;		// maximum allocated # of ints


string*	vc_strings		=0;		// vc string workspace
int		stralloc		=0;

int		vcreturn		=0;		// return value of last function
string	vcretstr		="";	// return value of last string function
int		returning_type	=0;		// hack to discern int from string returns

char*	movescriptbuf	=0;		// VC EntityMove buffer
char	vctrack			=0;		// VC call tracking to verge.log

quad*	vcstack			=0;		// VC stack (seperate from main stack)
quad*	vcsp			=0;		// VC stack pointer [esp]

int		mapevents		=0;		// number of map events in this VC
int*	event_offsets	=0;		// map VC offset table

int		hookretrace		=0;
int		hooktimer		=0;
int		invc			=0;

char	kill			=0;

// FUNC/STR/VAR ARRAYS

funcdecl*	funcs		=0;
int			numfuncs	=0;

strdecl*	str			=0;
int			numstr		=0;

vardecl*	vars		=0;
int			numvars		=0;

static int int_stack[1024+20];
static int int_stack_base=0, int_stack_ptr=0;

static string str_stack[1024+20];
static int str_stack_base=0, str_stack_ptr=0;

static int int_base[1024+20];
static int str_base[1024+20];
static int base_ptr=0;

static int int_last_base=0;
static int str_last_base=0;

static void PushBase(int ip, int sp)
{
	if (base_ptr<0 || base_ptr>=1024)
		err("PushBase: DEI!");

	int_base[base_ptr]=int_stack_base=ip;
	str_base[base_ptr]=str_stack_base=sp;

	base_ptr++;
}

static void PopBase()
{
	if (base_ptr<=0 || base_ptr>1024)
		err("PushBase: DEI!");

	base_ptr--;

	int_stack_base=int_base[base_ptr];
	str_stack_base=str_base[base_ptr];
}

static void PushInt(int n)
{
	if (int_stack_ptr<0 || int_stack_ptr>=1024)
		err("PushInt: DEI!");
	int_stack[int_stack_ptr]=n;
	int_stack_ptr++;
}
static int PopInt()
{
	if (int_stack_ptr<=0 || int_stack_ptr>1024)
		err("PopInt: DEI!");
	--int_stack_ptr;
	return int_stack[int_stack_ptr];
}

static void PushStr(string s)
{
	if (str_stack_ptr<0 || str_stack_ptr>1024)
		err("PushStr: DEI!");
	str_stack[str_stack_ptr++]=s;
}
static string PopStr()
{
	if (str_stack_ptr<=0 || str_stack_ptr>1024)
		err("PopStr: DEI!");
	--str_stack_ptr;
	return str_stack[str_stack_ptr];
}

// PROTOTYPES /////////////////////////////////////////////////////////////////////////////////////

string ResolveString();
void ExecuteSection();
void ExecuteEvent(int i);
void ExecuteUserFunc(int i);

int ProcessOperand();                 // Mutually dependant functions suck.
int ProcessIfOperand();               // Hell yeah they do, bitch.
void HandleExternFunc();
void HandleStdLib();
void ExecuteBlock();

// CODE ///////////////////////////////////////////////////////////////////////////////////////////

int bindarray[256] = { 0 };

static int sys_codesize=0;
static char* absolute_sys=0;

static char xvc_sig[8] = "VERGE2X";

void LoadSystemVC()
{
	VFILE*	f;
	char buf[8];
	int n;

	log("Initializing VC interpreter");
	// open system variable and function index
	f=vopen("system.xvc");
	if (!f)
		err("Could not open system.xvc");

	vread(buf, 8, f);
	if (strncmp(buf, xvc_sig, 8))
		err("LoadSystemIndex: system.xvc contains invalid signature");

	vread(&n, 4, f); // skip something.. code offset I guess!
	
	vread(&numvars, 4, f);
	vars = (vardecl *) malloc(numvars * sizeof vardecl);
	vread(vars, numvars * sizeof vardecl, f);

	vread(&numfuncs, 4, f);
	funcs = (funcdecl *) malloc(numfuncs * sizeof funcdecl);
	vread(funcs, numfuncs * sizeof funcdecl, f);
		
	vread(&numstr, 4, f);
	str = (strdecl *) malloc(numstr * sizeof strdecl);
	vread(str, numstr * sizeof strdecl, f);
	
	// grab system script code size and allocate a buffer for it
	sys_codesize = filesize(f);
	sysvc = new char[sys_codesize];
	absolute_sys = sysvc;

	// how many funcs, global ints, and global strings?
	vread(&numfuncs, 4, f);	
	vread(&maxint, 4, f);
	vread(&stralloc, 4, f);

	// allocate global integer and string arrays
	if (maxint)
		globalint = new int[maxint];
	if (stralloc)
		vc_strings = new string[stralloc];

	// important! initialize all ints to 0.
	memset(globalint, 0, maxint*4);

	// read in system script code
	vread(sysvc, sys_codesize, f);
	vclose(f);

	// initialize VC stack
	vcstack = new quad[1500];
	vcsp = vcstack;

	movescriptbuf = new char[256 * 256];

	log("system vclib init: %d funcs, %d ints (%d bytes), %d strings",
		numfuncs, numvars, maxint*4, numstr, stralloc);
}


void RunVCAutoexec()
{
	int n;

	for (n=0; n<numfuncs; n++)
	{
		char *x = funcs[n].fname;
		strlwr(x);
		if (!strcmp(x,"autoexec"))
			break;
	}
	if (n<numfuncs)
		ExecuteUserFunc(n);
	else
		err("No AutoExec() found in system scripts.");
}

static int map_codesize=0;
static char* absolute_map=0;

void LoadMapVC(VFILE *f)
{
	vread(&mapevents, 4, f);
	if (event_offsets)
		delete[] event_offsets;
	event_offsets=new int[mapevents];
	if (!event_offsets)
		err("LoadMapVC: memory exhausted on event_offsets");
	vread(event_offsets, 4*mapevents, f);

	vread(&map_codesize, 4, f);

log("%s: mapevents %d,  codesize %d", mapname, mapevents, map_codesize);
	mapvc=new char[map_codesize];
	absolute_map=mapvc;
	vread(mapvc, map_codesize, f);
/*
	for (int i=0; i<numstr; i++)
		if (!strcmp("mapname",str[i].vname))
		{
			vc_strings[i] = the_map;
			return;
		}
		*/ // FIXME!
}

void FreeMapVC()
{
	memset(bindarray, 0, sizeof bindarray);

	/* release memory */
	delete[] event_offsets;
	delete[] mapvc;
	event_offsets = 0;
}

byte GrabC()
{
	return *code++;
}

word GrabW(void)
{
	code+=2;
	return *(word *)(code-2);
}

quad GrabD(void)
{
	code+=4;
	return *(quad *)(code-4);
}

string GrabString()
{
	string	ret;
	int		c;
	char	temp[32+1];

	ret="";
	c=0;
	while (*code)
	{
		temp[c++]=GrabC();
		if (c>=32)
		{
			c=temp[c]='\0';
			ret+=temp;
		}
	}
	if (c)
	{
		temp[c]='\0';
		ret+=temp;
	}
	code++;

	return ret;
}

static int volume_hack = 100;
int ReadInt(char category, int loc, int ofs)
{
	static char me2vc[] = { 0, 1, 0, 2, 3 };
	
	switch (category)
	{
	case op_UVAR:
		if (loc<0 || loc>=maxint)
			err("ReadInt: bad offset to globalint (var)");
		return globalint[loc];
	case op_UVARRAY:
		if (loc<0 || loc>=maxint)
			err("ReadInt: bad offset to globalint (arr)");
		return globalint[loc];
	case op_HVAR0:
		switch (loc)
		{
			case 0: return xwin;
			case 1: return ywin;
			case 2: return cameratracking;
			case 3: return vctimer;
			case 4: return up;
			case 5: return down;
			case 6: return left;
			case 7: return right;
			case 8: return b1;
			case 9: return b2;
			case 10: return b3;
			case 11: return b4;
			case 12: return myscreen->width;
			case 13: return myscreen->height;
			case 14: return player;
//			case 15: return cc;  ?????
			case 16: return tracker;
			case 17: return mouse_x;
			case 18: return mouse_y;
			case 19: return mouse_l | (mouse_r << 1);
			case 20: return vctrack;
			case 21: return width;
			case 22: return depth;
			case 23: return volume_hack;  // FIXME: volume crap
			case 24: return (int) current_map->vsp->vspspace;
			case 25: return 0; // lastent
			case 26: return lastpressed;
			case 27: return current_map->mapwidth();
			case 28: return current_map->mapheight();
			case 29: return 0; // vsync?
			case 30: return entities; 
			default: err("Unknown HVAR0 (%d)", loc); 
		}
	case op_HVAR1:
		switch (loc)
		{
			case 1:
				if (ofs<0 || ofs>=entities)
					if (vc_paranoid) err("entity.x: no such entity, pal. (%d)", ofs);
					else return 0; 
				return entity[ofs]->getx();
			case 2:
				if (ofs<0 || ofs>=entities) 
					if (vc_paranoid) err("entity.y: no such entity, pal. (%d)", ofs);
					else return 0;
				return entity[ofs]->gety();
			case 3: 
				if (ofs<0 || ofs>=entities) 
					if (vc_paranoid) err("entity.tx: no such entity, pal. (%d)", ofs);
					else return 0;
				return entity[ofs]->getx()/16;
			case 4:
				if (ofs<0 || ofs>=entities)
					if (vc_paranoid) err("entity.ty: no such entity, pal. (%d)", ofs);
					else return 0;
				return entity[ofs]->gety()/16;
			case 5:
				if (ofs<0 || ofs>=entities)
					if (vc_paranoid) err("entity.facing: no such entity, pal. (%d)", ofs);
					else return 0;
				return me2vc[entity[ofs]->face];
			case 6:
				if (ofs<0 || ofs>=entities)
					if (vc_paranoid) err("entity.moving: no such entity, pal. (%d)", ofs);
					else return 0;
				return !entity[ofs]->ready();
			case 7:
				if (ofs<0 || ofs>=entities) 
					if (vc_paranoid) err("entity.specframe: no such entity, pal. (%d)", ofs);
					else return 0;
				return entity[ofs]->specframe;
			case 8:
				if (ofs<0 || ofs>=entities) 
					if (vc_paranoid) err("entity.speed: no such entity, pal. (%d)", ofs);
					else return 0;
				switch (entity[ofs]->getspeed())
				{
					case 0: return 0;
					case 25: return 1;
					case 33: return 2;
					case 50: return 3;
					case 100: return 4;
					case 200: return 5;
					case 300: return 6;
					case 400: return 7;
				}
				return 0;
			case 9: 
				if (ofs<0 || ofs>=entities)
					if (vc_paranoid) err("entity.movecode: no such entity, pal. (%d)", ofs);
				else return 0;
				return entity[ofs]->movecode;
			case 11: return keys[ofs];

			case 13: return (int) (*(byte *)ofs);
			case 14: return (int) (*(word *)ofs);
			case 15: return (int) (*(quad *)ofs);
			case 16:
				if (ofs<0 || ofs>768) return 0;
				return base_pal8[ofs]*63/255;
			case 17: return (int) (*(char *)ofs);
			case 18: return (int) (*(short*)ofs);
			case 19: return (int) (*(int  *)ofs);
			case 20: return 1; // entity.isob
			case 21: return 1; // entity.canob
			case 22: return 1; // entity.autoface
			case 23:
				if (ofs<0 || ofs>=entities)
					if (vc_paranoid) err("entity.visible: no such entity, pal. (%d)", ofs);
					else return 0;
				return entity[ofs]->visible ? 1 : 0;
			case 24:
				if (ofs<0 || ofs>=entities)
					if (vc_paranoid) err("entity.on: no such entity, pal. (%d)", ofs);
					else return 0;
				return entity[ofs]->active ? 1 : 0;
			default: err("Unknown HVAR1 (%d, %d)", loc, ofs);
		}
		break;
	case op_LVAR:
		if (loc<0 || loc>19)
			err("ReadInt: bad offset to local ints: %d", loc);		
		return int_stack[int_stack_base+loc];

	default:
		err("VC Execution error: Invalid ReadInt category %d", (int) category);
	}
	return 0;
}

void WriteInt(char category, int loc, int ofs, int value)
{
	static char vc2me[] = { 2, 1, 3, 4 };
	switch (category)
	{
	case op_UVAR:
		if (loc<0 || loc>=maxint)
			err("WriteInt: bad offset to variable globalint (%d)", loc);
		globalint[loc]=value; break;
	case op_UVARRAY:
		if (loc<0 || loc>=maxint)
			err("WriteInt: bad offset to array globalint (%d)", loc);
		globalint[loc]=value; break;
	case op_HVAR0:
		switch (loc)
		{
			case 0: xwin = value; return;
			case 1: ywin = value; return;
			case 2: cameratracking = value; return;
			case 3: vctimer = value; return;
			case 14: player = value; return;
			case 16: tracker = value; return;
			case 20: vctrack = value; return;
			case 23: volume_hack = value; return;
			case 26: lastpressed = value; return;
			default: err("Unknown HVAR0 (%d) (set %d)", loc, value);
		}
	case op_HVAR1:
		switch (loc)
		{
			case 1: 
				if (ofs<0 || ofs>=entities) 
					if (vc_paranoid) err("entity.x: no such entity, pal. (%d set: %d)", ofs, value);
					else return;
				entity[ofs]->setxy(value, entity[ofs]->gety());
				break;
			case 2:
				if (ofs<0 || ofs>=entities)
					if (vc_paranoid) err("entity.y: no such entity, pal. (%d set: %d)", ofs, value);
					else return;
				entity[ofs]->setxy(entity[ofs]->getx(), value);
				break;			
			case 3: return; // entity tx
			case 4: return; // entity ty
			case 5:
				if (ofs<0 || ofs>=entities)
					if (vc_paranoid) err("entity.facing: no such entity, pal. (%d)", ofs);
					else return;
				entity[ofs]->face = vc2me[value];
				return;
			case 6:
				if (ofs<0 || ofs>=entities)
					if (vc_paranoid) err("entity.moving: no such entity, pal. (%d)", ofs);
					else return;
				if (!value) entity[ofs]->stop();
				return;
			case 7:
				if (ofs<0 || ofs>=entities) 
					if (vc_paranoid) err("entity.specframe: no such entity, pal. (%d set: %d)", ofs, value);
					else return;
				entity[ofs]->specframe = value;
				return;
			case 8:
				if (ofs<0 || ofs>=entities) 
					if (vc_paranoid) err("entity.speed: no such entity, pal. (%d set: %d)", ofs, value);
					else return;
				switch (value)
				{
					case 0: entity[ofs]->setspeed(0);
					case 1: entity[ofs]->setspeed(25);
					case 2: entity[ofs]->setspeed(33);
					case 3: entity[ofs]->setspeed(50);
					case 4: entity[ofs]->setspeed(100);
					case 5: entity[ofs]->setspeed(200);
					case 6: entity[ofs]->setspeed(300);
					case 7: entity[ofs]->setspeed(400);
				}
				return;
			case 9: if (ofs<0 || ofs>=entities) 
						if (vc_paranoid) err("entity.movecode: no such entity, pal. (%d set: %d)", ofs, value);
					else return;
				if (!value)
					entity[ofs]->movecode = 0; 
					return; 
			case 11: keys[ofs] = value; return;

			case 13: (*(byte *)ofs)=(byte) value; return;
			case 14: (*(word *)ofs)=(word) value; return;
			case 15: (*(quad *)ofs)=(quad) value; return;
			case 16: 
				if (ofs<0 || ofs>768) return;
				base_pal8[ofs] = value*255/63;
				base_pal[ofs/3] = MakeColor(base_pal8[ofs*3], base_pal8[(ofs*3)+1], base_pal8[(ofs*3)+2]);
				return;
			case 17: (*(char *)ofs)=(byte) value; return;
            case 18: (*(short*)ofs)=(word) value; return;
            case 19: (*(int  *)ofs)=(quad) value; return;
			case 20:
				log("entity.isob set %d=%d", ofs, value);
				return;
			case 21:
				log("entity.canob set %d=%d", ofs, value);
				return;
			case 22:
				log("entity.autoface set %d=%d", ofs, value);
				return;
			case 23:
				if (ofs<0 || ofs>=entities) 
					if (vc_paranoid) err("entity.visible: no such entity, pal. (%d set: %d)", ofs, value);
					else return;
				entity[ofs]->visible = value ? true : false;
				return;
			case 24:
				if (ofs<0 || ofs>=entities) 
					if (vc_paranoid) err("entity.on: no such entity, pal. (%d set: %d)", ofs, value);
					else return;
				entity[ofs]->active = value ? true : false;
				return;
			default: err("Unknown HVAR1 (%d, %d) (set %d)", loc, ofs, value);
		}
		break;
	case op_LVAR:
		if (loc<0 || loc>19)
			err("WriteInt: bad offset to local ints: %d", loc);
		int_stack[int_stack_base+loc] = value;
		return;

	default:
		err("VC Execution error: Invalid WriteInt category %d", (int) category);
	}
}

int ResolveOperand()
{
	int cr=0;
	int d=0;
	byte c=0;

	cr=ProcessOperand();	// Get base number
	while (1)
	{
		c=GrabC();
		switch (c)
		{
		case op_ADD: cr += ProcessOperand(); continue;
		case op_SUB: cr -= ProcessOperand(); continue;
		case op_DIV:
			d=ProcessOperand();
			if (!d) cr=0; else cr /= d;
			continue;
		case op_MULT: cr = cr * ProcessOperand(); continue;
		case op_MOD:
			d=ProcessOperand();
			if (!d) cr=0; else cr %= d;
			continue;
		case op_SHL: cr = cr << ProcessOperand(); continue;
		case op_SHR: cr = cr >> ProcessOperand(); continue;
		case op_AND: cr = cr & ProcessOperand(); continue;
		case op_OR:  cr = cr | ProcessOperand(); continue;
		case op_XOR: cr = cr ^ ProcessOperand(); continue;
		case op_END: break;
		}
		break;
	}
	return cr;
}

int ProcessOperand()
{
	byte op_desc=0;
	byte c=0;
	quad d=0;
	quad ofs=0;

	op_desc=GrabC();
	switch (op_desc)
	{
		case op_IMMEDIATE: return GrabD();
		case op_HVAR0: c=GrabC(); return ReadInt(op_HVAR0, c, 0);
		case op_HVAR1: c=GrabC(); ofs=ResolveOperand(); return ReadInt(op_HVAR1, c, ofs);
		case op_UVAR:  d=GrabD(); return ReadInt(op_UVAR, d, 0);
		case op_UVARRAY: d=GrabD(); d+=ResolveOperand(); return ReadInt(op_UVARRAY, d, 0);
		case op_LVAR:
			c=GrabC();
			if (c>19) err("ProcessOperand: bad offset to local ints %d", c);
			return int_stack[int_stack_base+c];
		case op_BFUNC:
			HandleStdLib();
			return vcreturn;
		case op_UFUNC:
			HandleExternFunc();
			return vcreturn;
		case op_GROUP: return ResolveOperand();
		default:
			err("VC Execution error: Invalid operand %d.", op_desc);
			break;
	}
	return 0;
}

string HandleStringOperand()
{
	string	ret;
	int		c;

	c=GrabC();
	switch (c)
	{
	case s_IMMEDIATE:
		ret=GrabString();
		break;

	case s_GLOBAL:
		c=GrabW();
		if (c>=0 && c<stralloc)
			ret=vc_strings[c];
		else
			err("HandleStringOperand: bad offset to vc_strings");
		break;

	case s_ARRAY:
		c=GrabW();
		c+=ResolveOperand();
		if (c>=0 || c<stralloc)
			ret=vc_strings[c];
		else
			err("HandleStringOperand: bad offset to vc_strings");
		break;

	case s_NUMSTR:
		ret=va("%d", ResolveOperand());
		break;

	case s_LEFT:
		ret=ResolveString();
		ret=ret.left(ResolveOperand());
		break;

	case s_RIGHT:
		ret=ResolveString();
		ret=ret.right(ResolveOperand());
		break;

	case s_MID:
		ret=ResolveString();
		c=ResolveOperand();
		ret=ret.mid(c, ResolveOperand());
		break;

	case s_CHR:
		ret=(char)ResolveOperand();
		break;

	case s_LOCAL:
		c=GrabC();
		if (c>=0 && c<20)
			ret = str_stack[str_stack_base+c];
		else
			err("HandleStringOperand: bad offset to local strings");
		break;

	case s_UFUNC:
		HandleExternFunc();
		ret=vcretstr;
		break;

	default:
		err("Invalid VC string operand %d", c);
	}

	return ret;
}

string ResolveString()
{
	string	ret;
	int		c;

	ret=HandleStringOperand();
	do
	{
		c=GrabC();
		if (s_ADD==c)
			ret+=HandleStringOperand();
		else if (s_END!=c)
			err("VC execution error: Unknown string operator %d", c);
	} while (c!=s_END);

	return ret;
}

void vcpush(quad info)
{
	if (vcsp >= vcstack+1500)
		err("VC stack overflow.");

	*vcsp++ = info;
}

quad vcpop()
{
	if (vcsp <= vcstack)
		err("VC stack underflow.");

	return *--vcsp;
}

#include "libvc.h"

// ========================== VC Interpretation Core ==========================

int ProcessIf()
{
	byte exec, c;

	exec=(byte)ProcessIfOperand();	// Get base value;
	while (1)
	{
		c=GrabC();
		switch (c)
		{
			case i_AND: exec=(byte)(exec & ProcessIfOperand()); continue;
			case i_OR: exec=(byte)(exec | ProcessIfOperand()); continue;
			case i_UNGROUP: break;
		}
		break;
	}
	return exec;
}

int ProcessIfOperand()
{
	byte op_desc;
	int eval;

	eval=ResolveOperand();
	op_desc=GrabC();
	switch (op_desc)
	{
		case i_ZERO: if (!eval) return 1; else return 0;
		case i_NONZERO: if (eval) return 1; else return 0;
		case i_EQUALTO: if (eval == ResolveOperand()) return 1; else return 0;
		case i_NOTEQUAL: if (eval != ResolveOperand()) return 1; else return 0;
		case i_GREATERTHAN: if (eval > ResolveOperand()) return 1; else return 0;
		case i_GREATERTHANOREQUAL: if (eval >= ResolveOperand()) return 1; else return 0;
		case i_LESSTHAN: if (eval < ResolveOperand()) return 1; else return 0;
		case i_LESSTHANOREQUAL: if (eval <= ResolveOperand()) return 1; else return 0;
		case i_GROUP: if (ProcessIf()) return 1; else return 0;
	}

	return 0;
}

void HandleIf()
{
	char *d;

	if (ProcessIf())
	{
		GrabD();
		return;
	}
	d		=(char *)GrabD();
	code	=(char *)(int)basevc+(int)d;
}

static int routine_depth=0;

void HandleExternFunc()
{
	funcdecl*	pfunc;
	int		n;
	int ilb=0, slb=0;

	n=GrabW();
	if (n<0 || n>=numfuncs)
	{
		err("HandleExternFunc: VC sys script out of bounds (%d/%d)", n, numfuncs);
	}
	pfunc=funcs+n;

	ilb=int_last_base;
	slb=str_last_base;
	PushBase(int_last_base, str_last_base);

	int		isp, ssp;

	// we do not set the new base until we're done reading in the arguments--
	// this is because we might still need to read in local vars passed from the
	// previous function (lookup for locals works off current base values).
	// for now, just tag the to-be bases.
		isp	=int_stack_ptr;
		ssp	=str_stack_ptr;
// allocate stack space
	if (pfunc->numlocals)
	{
	// read in arguments
		for (n=0; n<pfunc->numargs; n++)
		{
			switch (pfunc->argtype[n])
			{
				case v_INT:
					PushInt(ResolveOperand());
					break;
				case v_STRING:
					PushStr(ResolveString());
					break;
			}
		}
	// finish off allocating locals
		while (n<pfunc->numlocals)
		{
			switch (pfunc->argtype[n])
			{
				case v_INT:
					PushInt(0);
					break;
				case v_STRING:
					PushStr("");
					break;
			}
			n++;
		}
	}
	// now we're safe to set the bases
	int_stack_base=int_last_base=isp;
	str_stack_base=str_last_base=ssp;

	vcpush((quad)basevc);
	vcpush((quad)code);

	basevc	=sysvc;
	code	=(char *)(basevc + pfunc->syscodeofs);

	if (vctrack)
	{
//		for (n=0; n<routine_depth; n++)
//			Logp("  ");
		log(va(" --> Entering user func %s, codeofs %d",
			pfunc->fname, pfunc->syscodeofs));
		routine_depth++;
	}

	ExecuteBlock();

	basevc	=(char *)vcpop();

	// restore previous base
		PopBase();
		int_last_base=ilb;
		str_last_base=slb;
// free stack space
	if (pfunc->numlocals)
	{
	// clear out all locals (args + 'true' locals)
		for (n=0; n<pfunc->numlocals; n++)
		{
			switch (pfunc->argtype[n])
			{
				case v_INT:
					PopInt();
					break;
				case v_STRING:
					PopStr();
					break;
			}
		}
	}

	if (vctrack)
	{
		routine_depth--;
//		for (n=0; n<routine_depth; n++)
//			Logp("  ");
		log(va(" --> Returned from %s", pfunc->fname));
	}
}

void HandleAssign()
{
	int		op, c, base, offset, value;

	c=GrabC();

// string assignment
	if (c == op_STRING)
	{
		offset=GrabW();
		c=GrabC();
		if (c != a_SET)
			err("VC execution error: Corrupt string assignment");
		if (offset>=0 && offset<stralloc)
			vc_strings[offset]=ResolveString();
		else
			err("HandleAssign: bad offset to vc_strings (var)");
		return;
	}
// string array assignment
	if (c == op_SARRAY)
	{
		offset=GrabW();
		offset+=ResolveOperand();
		c=GrabC();
		if (c != a_SET)
			err("VC execution error: Corrupt string assignment");
		if (offset>=0 && offset<stralloc)
			vc_strings[offset]=ResolveString();
		else
			err("HandleAssign: bad offset to vc_strings (arr)");
		return;
	}
// local string assignment
	if (c == op_SLOCAL)
	{
		offset=GrabW();
		c=GrabC();
		if (c != a_SET)
			err("VC execution error: Corrupt string assignment");
		if (offset>=0 && offset<20)
			str_stack[str_stack_base+offset]=ResolveString();
		else
			err("HandleAssign: bad offset to local strings");
		return;
	}

// integer assignment
	base=offset=0;
	switch (c)
	{
		case op_UVAR:		base=GrabD(); break;
		case op_UVARRAY:	base=GrabD(); base+=ResolveOperand(); break;
		case op_HVAR0:		base=GrabC(); break;
		case op_HVAR1:		base=GrabC(); offset=ResolveOperand(); break;
		case op_LVAR:		base=GrabC(); break;

		default:
			err("VC Execution error: Unknown assignment category.");
	}
	value=ReadInt((char)c, base, offset);
	op=GrabC();
	switch(op)
	{
		case a_SET:		value=ResolveOperand(); break;
		case a_INC:		value++; break;
		case a_DEC:		value--; break;
		case a_INCSET:	value+=ResolveOperand(); break;
		case a_DECSET:	value-=ResolveOperand(); break;

		default:
			err("VC Execution error: Invalid assignment operator %d.", op);
	}
	WriteInt((char)c, base, offset, value);
}

void HandleSwitch()
{
	int realvalue=0;
	int compvalue=0;
	byte c=0;
	byte *next=0;

	realvalue=ResolveOperand();
	c=GrabC();
	while (c != opRETURN)
	{
		compvalue=ResolveOperand();
		next=(byte *)GrabD();
		if (compvalue != realvalue)
		{
			code=(char *)(int)basevc+(int)next;
			c=GrabC();
			continue;
		}
		ExecuteSection();
		c=GrabC();
	}
}

void ExecuteVC()
{
	byte c=0;

	while (1)
	{
		if (kill) break;
//		UpdateControls();

		c=GrabC();
		switch (c)
		{
			case opEXEC_STDLIB: HandleStdLib(); break;
			case opEXEC_LOCALFUNC: break;
			case opEXEC_EXTERNFUNC: HandleExternFunc(); break;
			case opIF: HandleIf(); break;
			case opELSE: break;
			case opGOTO: code=basevc+GrabD(); break;
			case opSWITCH: HandleSwitch(); break;
			case opASSIGN: HandleAssign(); break;
			case opRETURN: code=(char *) vcpop(); break;
			case opSETRETVAL: vcreturn=ResolveOperand(); break;
			case opSETRETSTRING: vcretstr=ResolveString(); break;

			default:
				err("Internal VC execution error. (%d)", (int) code - (int) basevc);
		}

		if ((int)code != -1)
			continue;
		else
			break;
	}
}

void ExecuteBlock()
{
	byte c=0;

	while (1)
	{
		if (kill) break;
//		UpdateControls();

		c=GrabC();
		switch (c)
		{
			case opEXEC_STDLIB: HandleStdLib(); break;
			case opEXEC_LOCALFUNC: break;
			case opEXEC_EXTERNFUNC: HandleExternFunc(); break;
			case opIF: HandleIf(); break;
			case opELSE: break;
			case opGOTO: code=basevc+GrabD(); break;
			case opSWITCH: HandleSwitch(); break;
			case opASSIGN: HandleAssign(); break;
			case opRETURN: code=(char *) vcpop(); break;
			case opSETRETVAL: vcreturn=ResolveOperand(); break;
			case opSETRETSTRING: vcretstr=ResolveString(); break;

			default:
				err("Internal VC execution error. (%d)",(int) code - (int) basevc);
		}

		if (c != opRETURN)
			continue;
		else
			break;
	}
}

void ExecuteSection()
{
	byte c=0;

	while (1)
	{
		if (kill) break;
//		UpdateControls();

		c=GrabC();
		switch (c)
		{
			case opEXEC_STDLIB: HandleStdLib(); break;
			case opEXEC_LOCALFUNC: break;
			case opEXEC_EXTERNFUNC: HandleExternFunc(); break;
			case opIF: HandleIf(); break;
			case opELSE: break;
			case opGOTO: code=basevc+GrabD(); break;
			case opSWITCH: HandleSwitch(); break;
			case opASSIGN: HandleAssign(); break;
			case opRETURN: break;
			case opSETRETVAL: vcreturn=ResolveOperand(); break;
			case opSETRETSTRING: vcretstr=ResolveString(); break;

			default:
				err("Internal VC execution error. (%d)", (int) code - (int) basevc);
		}

		if (c != opRETURN)
			continue;
		else
			break;
	}
}

void ExecuteEvent(int ev)
{
	if (ev<0 || ev>=mapevents)
	{
		err("ExecuteEvent: VC event out of bounds (%d)", ev);
	}

	++invc;

	vcpush((quad)code);
	vcpush((quad)basevc);

	basevc	=mapvc;
	code	=basevc+event_offsets[ev];

	vcpush (-1);
	ExecuteVC();

	basevc	=(char *)vcpop();
	code	=(char *)vcpop();

	--invc;
}

void ExecuteUserFunc(int ufunc)
{
	funcdecl*	pfunc;
	int		n;
	int ilb=0, slb=0;
	
	if (ufunc<0 || ufunc>=numfuncs)
	{
		err("ExecuteUserFunc: VC sys script out of bounds (%d/%d)", ufunc, numfuncs);
	}
	pfunc=funcs+ufunc;

	ilb=int_last_base;
	slb=str_last_base;
	PushBase(int_last_base, str_last_base);

	int		isp, ssp;

	// we do not set the new base until we're done reading in the arguments--
	// this is because we might still need to read in local vars passed from the
	// previous function (lookup for locals works off current base values).
	// for now, just tag the to-be bases.
		isp	=int_stack_ptr;
		ssp	=str_stack_ptr;
// allocate stack space
	if (pfunc->numlocals)
	{
	// read in arguments
		for (n=0; n<pfunc->numargs; n++)
		{
			switch (pfunc->argtype[n])
			{
				case v_INT:
					PushInt(ResolveOperand());
					break;
				case v_STRING:
					PushStr(ResolveString());
					break;
			}
		}
	// finish off allocating locals
		while (n<pfunc->numlocals)
		{
			switch (pfunc->argtype[n])
			{
				case v_INT:
					PushInt(0);
					break;
				case v_STRING:
					PushStr("");
					break;
			}
			n++;
		}
	}
	// now we're safe to set the bases
	int_stack_base=int_last_base=isp;
	str_stack_base=str_last_base=ssp;

	vcpush((quad)basevc);
	vcpush((quad)code);

	basevc	=sysvc;
	code	=(char *)(basevc + pfunc->syscodeofs);

	if (vctrack)
	{
//		for (n=0; n<routine_depth; n++)
//			Logp("  ");
		log(va(" --> Entering user func %s, codeofs %d",
			pfunc->fname, pfunc->syscodeofs));
		routine_depth++;
	}

	ExecuteBlock();

	basevc	=(char *)vcpop();

	// restore previous base
		PopBase();
		int_last_base=ilb;
		str_last_base=slb;
// free stack space
	if (pfunc->numlocals)
	{
	// clear out all locals (args + 'true' locals)
		for (n=0; n<pfunc->numlocals; n++)
		{
			switch (pfunc->argtype[n])
			{
				case v_INT:
					PopInt();
					break;
				case v_STRING:
					PopStr();
					break;
			}
		}
	}

	if (vctrack)
	{
		routine_depth--;
//		for (n=0; n<routine_depth; n++)
//			Logp("  ");
		log(va(" --> Returned from %s", pfunc->fname));
	}
}

void HookRetrace()
{
	if (!hookretrace) return;

	if (hookretrace <  USERFUNC_MARKER)
		ExecuteEvent(hookretrace);
	if (hookretrace >= USERFUNC_MARKER)
		ExecuteUserFunc(hookretrace-USERFUNC_MARKER);
}

void HookTimer()
{
	if (!hooktimer) return;

	if (hooktimer <  USERFUNC_MARKER)
		ExecuteEvent(hooktimer);
	if (hooktimer >= USERFUNC_MARKER)
		ExecuteUserFunc(hooktimer-USERFUNC_MARKER);
}

void HookKey(int script)
{
	if (!script) return;
	if (script <  USERFUNC_MARKER)
		ExecuteEvent(script);
	if (script >= USERFUNC_MARKER)
		ExecuteUserFunc(script-USERFUNC_MARKER);
}
#undef G_VC