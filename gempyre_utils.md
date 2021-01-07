![wqe](https://avatars1.githubusercontent.com/u/7837709?s=400&amp;v=4)

gempyre_utils.h
=====
Gempyre GUI Framework
-------------

gempyre_utils.h contains a collection utility functions used internally within Gempyre
implementation and test applications.

* [GempyreUtils ](#gempyreutils)
* [inline std::string qq(const std::string& s) ](#inline-std-string-qq-const-std-string-s-)
* [inline std::string chop(const std::string& s) ](#inline-std-string-chop-const-std-string-s-)
* [inline std::string chop(const std::string& s, const std::string& chopped) ](#inline-std-string-chop-const-std-string-s-const-std-string-chopped-)
* [ std::string substitute(const std::string& str, const std::string& substring,  const std::string& substitution) ](#-std-string-substitute-const-std-string-str-const-std-string-substring-const-std-string-substitution-)
* [T to(const std::string& source) ](#t-to-const-std-string-source-)
* [std::optional<T> toOr(const std::string& source) ](#std-optionalt-toor-const-std-string-source-)
* [T toLow(const T& str) ](#t-tolow-const-t-str-)
* [std::string toHex(T ival) ](#std-string-tohex-t-ival-)
* [bool contains(const T& container, const std::string& s) ](#bool-contains-const-t-container-const-std-string-s-)
* [std::optional<T> at(const C& container, const std::string& s, unsigned index = 0) ](#std-optionalt-at-const-c-container-const-std-string-s-unsigned-index-0-)
* [T atOr(const C& container, const std::string& s, const T& defaultValue, unsigned index = 0) ](#t-ator-const-c-container-const-std-string-s-const-t-defaultvalue-unsigned-index-0-)
* [Container split(const std::string& str, const char splitChar = ' ') ](#container-split-const-std-string-str-const-char-splitchar-)
* [std::string joinPairs(const IT& begin, const IT& end, const std::string& startChar = "{", const std::string& endChar = "}", const std::string& divChar = ":" , const std::string& joinChar = "" ) ](#std-string-joinpairs-const-it-begin-const-it-end-const-std-string-startchar-const-std-string-endchar-const-std-string-divchar-const-std-string-joinchar-)
* [std::string joinPairs(const T& obj, const std::string& startChar = "{", const std::string endChar = "}", const std::string& divChar = ":" , const std::string& joinChar = "" ) ](#std-string-joinpairs-const-t-obj-const-std-string-startchar-const-std-string-endchar-const-std-string-divchar-const-std-string-joinchar-)
* [std::string join(const IT& begin, const IT& end, const std::string joinChar = "" ) ](#std-string-join-const-it-begin-const-it-end-const-std-string-joinchar-)
* [std::string join(const T& t, const std::string joinChar = "" ) ](#std-string-join-const-t-t-const-std-string-joinchar-)
* [T merge(const T& b1, const T& b2) ](#t-merge-const-t-b1-const-t-b2-)
* [T merge(const T& b1, const T& b2, Arg ...args) ](#t-merge-const-t-b1-const-t-b2-arg-args-)
* [  std::string hexify(const std::string& src, const std::string pat) ](#-std-string-hexify-const-std-string-src-const-std-string-pat-)
* [  std::string unhexify(const std::string& src) ](#-std-string-unhexify-const-std-string-src-)
* [ OS currentOS() ](#-os-currentos-)
* [inline bool doFatal(const std::string& txt, std::function<void()> f, const char* file, int line) ](#inline-bool-dofatal-const-std-string-txt-std-functionvoid-f-const-char-file-int-line-)
* [ std::string getLink(const std::string& fname) ](#-std-string-getlink-const-std-string-fname-)
* [ bool isDir(const std::string& fname) ](#-bool-isdir-const-std-string-fname-)
* [ std::string workingDir() ](#-std-string-workingdir-)
* [ std::string absPath(const std::string& rpath) ](#-std-string-abspath-const-std-string-rpath-)
* [ std::string pathPop(const std::string& filename, int steps = 1) ](#-std-string-pathpop-const-std-string-filename-int-steps-1-)
* [ std::vector<std::tuple<std::string, bool, std::string>> directory(const std::string& dirname) ](#-std-vectorstd-tuplestd-string-bool-std-string-directory-const-std-string-dirname-)
* [ std::string baseName(const std::string& filename) ](#-std-string-basename-const-std-string-filename-)
* [ std::string tempName() ](#-std-string-tempname-)
* [ std::string systemEnv(const std::string& env) ](#-std-string-systemenv-const-std-string-env-)
* [ bool isHiddenEntry(const std::string& filename) ](#-bool-ishiddenentry-const-std-string-filename-)
* [ bool isExecutable(const std::string& filename) ](#-bool-isexecutable-const-std-string-filename-)
* [ SSIZE_T fileSize(const std::string& filename) ](#-ssize_t-filesize-const-std-string-filename-)
* [ bool rename(const std::string& of, const std::string& nf) ](#-bool-rename-const-std-string-of-const-std-string-nf-)
* [ void removeFile(const std::string& filename) ](#-void-removefile-const-std-string-filename-)
* [ bool fileExists(const std::string& filename) ](#-bool-fileexists-const-std-string-filename-)
* [std::string writeToTemp(const T& data) ](#std-string-writetotemp-const-t-data-)
* [std::vector<T> slurp(const std::string& file, const size_t max = std::numeric_limits<size_t>::max()) ](#std-vectort-slurp-const-std-string-file-const-size_t-max-std-numeric_limitssize_t-max-)
* [ std::string slurp(const std::string& file, const size_t max = std::numeric_limits<size_t>::max()) ](#-std-string-slurp-const-std-string-file-const-size_t-max-std-numeric_limitssize_t-max-)

---
### GempyreUtils 
###### The LogLevel enum 

---

---
##### String Utils 
##### inline std::string qq(const std::string& s) 
###### *Param:* s 
###### *Return:*  
##### inline std::string chop(const std::string& s) 
###### *Param:* s 
###### *Return:*  
##### inline std::string chop(const std::string& s, const std::string& chopped) 
###### *Param:* s 
###### *Param:* chopped 
###### *Return:*  
#####  std::string substitute(const std::string& str, const std::string& substring,  const std::string& substitution) 
###### *Param:* str 
###### *Param:* substring 
###### *Param:* substitution 
###### *Return:*  
##### T to(const std::string& source) 
###### *Param:* source 
###### *Return:*  
##### std::optional<T> toOr(const std::string& source) 
###### *Param:* source 
###### *Return:*  
##### T toLow(const T& str) 
###### *Param:* str 
###### *Return:*  
##### std::string toHex(T ival) 
###### *Param:* ival 
###### *Return:*  

---

---
##### Container Utils 
##### bool contains(const T& container, const std::string& s) 
###### *Param:* container 
###### *Param:* s 
###### *Return:*  
##### std::optional<T> at(const C& container, const std::string& s, unsigned index = 0) 
###### *Param:* container 
###### *Param:* s 
###### *Param:* index 
###### *Return:*  
##### T atOr(const C& container, const std::string& s, const T& defaultValue, unsigned index = 0) 
###### *Param:* container 
###### *Param:* s 
###### *Param:* defaultValue 
###### *Param:* index 
###### *Return:*  
##### Container split(const std::string& str, const char splitChar = ' ') 
###### *Param:* str 
###### *Param:* splitChar 
###### *Return:*  
##### std::string joinPairs(const IT& begin, const IT& end, const std::string& startChar = "{", const std::string& endChar = "}", const std::string& divChar = ":" , const std::string& joinChar = "" ) 
###### *Param:* begin 
###### *Param:* end 
###### *Param:* startChar 
###### *Param:* endChar 
###### *Param:* divChar 
###### *Param:* joinChar 
###### *Return:*  
##### std::string joinPairs(const T& obj, const std::string& startChar = "{", const std::string endChar = "}", const std::string& divChar = ":" , const std::string& joinChar = "" ) 
###### *Param:* obj 
###### *Param:* startChar 
###### *Param:* endChar 
###### *Param:* divChar 
###### *Param:* joinChar 
###### *Return:*  
##### std::string join(const IT& begin, const IT& end, const std::string joinChar = "" ) 
###### *Param:* begin 
###### *Param:* end 
###### *Param:* joinChar 
###### *Return:*  
##### std::string join(const T& t, const std::string joinChar = "" ) 
###### *Param:* t 
###### *Param:* joinChar 
###### *Return:*  
##### T merge(const T& b1, const T& b2) 
###### *Param:* b1 
###### *Param:* b2 
###### *Return:*  
##### T merge(const T& b1, const T& b2, Arg ...args) 
###### *Param:* b1 
###### *Param:* b2 
###### *Param:* args 
###### *Return:*  

---

---
##### Misc Utils 
#####   std::string hexify(const std::string& src, const std::string pat) 
###### *Param:* src 
###### *Param:* pat 
###### *Return:*  
#####   std::string unhexify(const std::string& src) 
###### *Param:* src 
###### *Return:*  
###### The OS enum 
#####  OS currentOS() 
###### *Return:*  
###### osBrowser 
###### *Return:*  
##### inline bool doFatal(const std::string& txt, std::function<void()> f, const char* file, int line) 
###### *Param:* txt 
###### *Param:* f 
###### *Param:* file 
###### *Param:* line 
###### *Return:*  

---

---
##### File Utils 
#####  std::string getLink(const std::string& fname) 
###### *Param:* fname 
###### *Return:*  
#####  bool isDir(const std::string& fname) 
###### *Param:* fname 
###### *Return:*  
#####  std::string workingDir() 
###### *Return:*  
#####  std::string absPath(const std::string& rpath) 
###### *Param:* rpath 
###### *Return:*  
#####  std::string pathPop(const std::string& filename, int steps = 1) 
###### *Param:* filename 
###### *Return:*  
#####  std::vector<std::tuple<std::string, bool, std::string>> directory(const std::string& dirname) 
###### *Param:* dirname 
###### *Return:*  
#####  std::string baseName(const std::string& filename) 
###### *Param:* filename 
###### *Return:*  
#####  std::string tempName() 
###### *Return:*  
#####  std::string systemEnv(const std::string& env) 
###### *Param:* env 
###### *Return:*  
#####  bool isHiddenEntry(const std::string& filename) 
###### *Param:* filename 
###### *Return:*  
#####  bool isExecutable(const std::string& filename) 
###### *Param:* filename 
###### *Return:*  
#####  SSIZE_T fileSize(const std::string& filename) 
###### *Param:* filename 
###### *Return:*  
#####  bool rename(const std::string& of, const std::string& nf) 
###### *Param:* of 
###### *Param:* nf 
###### *Return:*  
#####  void removeFile(const std::string& filename) 
###### *Param:* filename 
#####  bool fileExists(const std::string& filename) 
###### *Param:* filename 
###### *Return:*  
##### std::string writeToTemp(const T& data) 
###### *Param:* data 
###### *Return:*  
##### std::vector<T> slurp(const std::string& file, const size_t max = std::numeric_limits<size_t>::max()) 
###### *Param:* file 
###### *Param:* max 
###### *Return:*  
#####  std::string slurp(const std::string& file, const size_t max = std::numeric_limits<size_t>::max()) 
###### *Param:* file 
###### *Param:* max 
###### *Return:*  

---
###### Generated by MarkdownMaker, (c) Markus Mertama 2018 
