![wqe](https://avatars1.githubusercontent.com/u/7837709?s=400&amp;v=4)

telex_utils.h
=====
Telex GUI Framework
-------------

telex_utils.h contains a collection utility functions used internally within Telex
implementation and test applications.

* [ namespace TelexUtils ](#telexutils)
    * [ inline std::string qq(const std::string&amp; s) ](#inline-stdstring-qqconst-stdstring-s)
    * [ inline std::string chop(const std::string&amp; s) ](#inline-stdstring-chopconst-stdstring-s)
    * [ inline std::string chop(const std::string&amp; s, const std::string&amp; chopped) ](#inline-stdstring-chopconst-stdstring-s-const-stdstring-chopped)
    * [  std::string substitute(const std::string&amp; str, const std::string&amp; substring,  const std::string&amp; substitution) ](#stdstring-substituteconst-stdstring-str-const-stdstring-substring-const-stdstring-substitution)
    * [ T to(const std::string&amp; source) ](#t-toconst-stdstring-source)
    * [ std::optional&lt;T&gt; toOr(const std::string&amp; source) ](#stdoptionalt-toorconst-stdstring-source)
    * [ T toLow(const T&amp; str) ](#t-tolowconst-t-str)
    * [ bool contains(const T&amp; container, const std::string&amp; s) ](#bool-containsconst-t-container-const-stdstring-s)
    * [ std::optional&lt;T&gt; at(const C&amp; container, const std::string&amp; s, unsigned index = 0) ](#stdoptionalt-atconst-c-container-const-stdstring-s-unsigned-index-0)
    * [ T atOr(const C&amp; container, const std::string&amp; s, const T&amp; defaultValue, unsigned index = 0) ](#t-atorconst-c-container-const-stdstring-s-const-t-defaultvalue-unsigned-index-0)
    * [ Container split(const std::string&amp; str, const char splitChar = &#39; &#39;) ](#container-splitconst-stdstring-str-const-char-splitchar-39-39)
    * [ std::string joinPairs(const IT&amp; begin, const IT&amp; end, const std::string&amp; startChar = &quot;{&quot;, const std::string&amp; endChar = &quot;}&quot;, const std::string&amp; divChar = &quot;:&quot; , const std::string&amp; joinChar = &quot;&quot; ) ](#stdstring-joinpairsconst-it-begin-const-it-end-const-stdstring-startchar-const-stdstring-endchar-const-stdstring-divchar-const-stdstring-joinchar)
    * [ std::string joinPairs(const T&amp; obj, const std::string&amp; startChar = &quot;{&quot;, const std::string endChar = &quot;}&quot;, const std::string&amp; divChar = &quot;:&quot; , const std::string&amp; joinChar = &quot;&quot; ) ](#stdstring-joinpairsconst-t-obj-const-stdstring-startchar-const-stdstring-endchar-const-stdstring-divchar-const-stdstring-joinchar)
    * [ std::string join(const IT&amp; begin, const IT&amp; end, const std::string joinChar = &quot;&quot; ) ](#stdstring-joinconst-it-begin-const-it-end-const-stdstring-joinchar)
    * [ std::string join(const T&amp; t, const std::string joinChar = &quot;&quot; ) ](#stdstring-joinconst-t-t-const-stdstring-joinchar)
    * [ T merge(const T&amp; b1, const T&amp; b2) ](#t-mergeconst-t-b1-const-t-b2)
    * [ T merge(const T&amp; b1, const T&amp; b2, Arg ...args) ](#t-mergeconst-t-b1-const-t-b2-arg-args)
    * [   std::string hexify(const std::string&amp; src, const std::string pat) ](#stdstring-hexifyconst-stdstring-src-const-stdstring-pat)
    * [   std::string unhexify(const std::string&amp; src) ](#stdstring-unhexifyconst-stdstring-src)
    * [  OS currentOS() ](#os-currentos)
    * [ inline bool doFatal(const std::string&amp; txt, std::function&lt;void()&gt; f, const char* file, int line) ](#inline-bool-dofatalconst-stdstring-txt-stdfunctionvoid-f-const-char-file-int-line)
    * [  std::string getLink(const std::string&amp; fname) ](#stdstring-getlinkconst-stdstring-fname)
    * [  bool isDir(const std::string&amp; fname) ](#bool-isdirconst-stdstring-fname)
    * [  std::string workingDir() ](#stdstring-workingdir)
    * [  std::string absPath(const std::string&amp; rpath) ](#stdstring-abspathconst-stdstring-rpath)
    * [  std::string pathPop(const std::string&amp; filename) ](#stdstring-pathpopconst-stdstring-filename)
    * [  std::vector&lt;std::tuple&lt;std::string, bool, std::string&gt;&gt; directory(const std::string&amp; dirname) ](#stdvectorstdtuplestdstring-bool-stdstring-directoryconst-stdstring-dirname)
    * [  std::string tempName() ](#stdstring-tempname)
    * [  std::string systemEnv(const std::string&amp; env) ](#stdstring-systemenvconst-stdstring-env)
    * [  bool isHiddenEntry(const std::string&amp; filename) ](#bool-ishiddenentryconst-stdstring-filename)
    * [  bool isExecutable(const std::string&amp; filename) ](#bool-isexecutableconst-stdstring-filename)
    * [  SSIZE_T fileSize(const std::string&amp; filename) ](#ssize_t-filesizeconst-stdstring-filename)
    * [  bool rename(const std::string&amp; of, const std::string&amp; nf) ](#bool-renameconst-stdstring-of-const-stdstring-nf)
    * [  void removeFile(const std::string&amp; filename) ](#void-removefileconst-stdstring-filename)
    * [  bool fileExists(const std::string&amp; filename) ](#bool-fileexistsconst-stdstring-filename)
    * [ std::string writeToTemp(const T&amp; data) ](#stdstring-writetotempconst-t-data)
    * [ std::vector&lt;T&gt; slurp(const std::string&amp; file, const size_t max = std::numeric_limits&lt;size_t&gt;::max()) ](#stdvectort-slurpconst-stdstring-file-const-size_t-max-stdnumeric_limitssize_tmax)
    * [  std::string slurp(const std::string&amp; file, const size_t max = std::numeric_limits&lt;size_t&gt;::max()) ](#stdstring-slurpconst-stdstring-file-const-size_t-max-stdnumeric_limitssize_tmax)

---
<a id="TelexUtils"></a>
### TelexUtils 
###### The LogLevel enum 

---

---
##### String Utils 
<a id="inline-stdstring-qqconst-stdstring-s"></a>
##### inline std::string qq(const std::string&amp; s) 
###### *Param:* s 
###### *Return:*  
<a id="inline-stdstring-chopconst-stdstring-s"></a>
##### inline std::string chop(const std::string&amp; s) 
###### *Param:* s 
###### *Return:*  
<a id="inline-stdstring-chopconst-stdstring-s-const-stdstring-chopped"></a>
##### inline std::string chop(const std::string&amp; s, const std::string&amp; chopped) 
###### *Param:* s 
###### *Param:* chopped 
###### *Return:*  
<a id="stdstring-substituteconst-stdstring-str-const-stdstring-substring-const-stdstring-substitution"></a>
#####  std::string substitute(const std::string&amp; str, const std::string&amp; substring,  const std::string&amp; substitution) 
###### *Param:* str 
###### *Param:* substring 
###### *Param:* substitution 
###### *Return:*  
<a id="t-toconst-stdstring-source"></a>
##### T to(const std::string&amp; source) 
###### *Param:* source 
###### *Return:*  
<a id="stdoptionalt-toorconst-stdstring-source"></a>
##### std::optional&lt;T&gt; toOr(const std::string&amp; source) 
###### *Param:* source 
###### *Return:*  
<a id="t-tolowconst-t-str"></a>
##### T toLow(const T&amp; str) 
###### *Param:* str 
###### *Return:*  

---

---
##### Container Utils 
<a id="bool-containsconst-t-container-const-stdstring-s"></a>
##### bool contains(const T&amp; container, const std::string&amp; s) 
###### *Param:* container 
###### *Param:* s 
###### *Return:*  
<a id="stdoptionalt-atconst-c-container-const-stdstring-s-unsigned-index-0"></a>
##### std::optional&lt;T&gt; at(const C&amp; container, const std::string&amp; s, unsigned index = 0) 
###### *Param:* container 
###### *Param:* s 
###### *Param:* index 
###### *Return:*  
<a id="t-atorconst-c-container-const-stdstring-s-const-t-defaultvalue-unsigned-index-0"></a>
##### T atOr(const C&amp; container, const std::string&amp; s, const T&amp; defaultValue, unsigned index = 0) 
###### *Param:* container 
###### *Param:* s 
###### *Param:* defaultValue 
###### *Param:* index 
###### *Return:*  
<a id="container-splitconst-stdstring-str-const-char-splitchar-39-39"></a>
##### Container split(const std::string&amp; str, const char splitChar = &#39; &#39;) 
###### *Param:* str 
###### *Param:* splitChar 
###### *Return:*  
<a id="stdstring-joinpairsconst-it-begin-const-it-end-const-stdstring-startchar-const-stdstring-endchar-const-stdstring-divchar-const-stdstring-joinchar"></a>
##### std::string joinPairs(const IT&amp; begin, const IT&amp; end, const std::string&amp; startChar = &quot;{&quot;, const std::string&amp; endChar = &quot;}&quot;, const std::string&amp; divChar = &quot;:&quot; , const std::string&amp; joinChar = &quot;&quot; ) 
###### *Param:* begin 
###### *Param:* end 
###### *Param:* startChar 
###### *Param:* endChar 
###### *Param:* divChar 
###### *Param:* joinChar 
###### *Return:*  
<a id="stdstring-joinpairsconst-t-obj-const-stdstring-startchar-const-stdstring-endchar-const-stdstring-divchar-const-stdstring-joinchar"></a>
##### std::string joinPairs(const T&amp; obj, const std::string&amp; startChar = &quot;{&quot;, const std::string endChar = &quot;}&quot;, const std::string&amp; divChar = &quot;:&quot; , const std::string&amp; joinChar = &quot;&quot; ) 
###### *Param:* obj 
###### *Param:* startChar 
###### *Param:* endChar 
###### *Param:* divChar 
###### *Param:* joinChar 
###### *Return:*  
<a id="stdstring-joinconst-it-begin-const-it-end-const-stdstring-joinchar"></a>
##### std::string join(const IT&amp; begin, const IT&amp; end, const std::string joinChar = &quot;&quot; ) 
###### *Param:* begin 
###### *Param:* end 
###### *Param:* joinChar 
###### *Return:*  
<a id="stdstring-joinconst-t-t-const-stdstring-joinchar"></a>
##### std::string join(const T&amp; t, const std::string joinChar = &quot;&quot; ) 
###### *Param:* t 
###### *Param:* joinChar 
###### *Return:*  
<a id="t-mergeconst-t-b1-const-t-b2"></a>
##### T merge(const T&amp; b1, const T&amp; b2) 
###### *Param:* b1 
###### *Param:* b2 
###### *Return:*  
<a id="t-mergeconst-t-b1-const-t-b2-arg-args"></a>
##### T merge(const T&amp; b1, const T&amp; b2, Arg ...args) 
###### *Param:* b1 
###### *Param:* b2 
###### *Param:* args 
###### *Return:*  

---

---
##### Misc Utils 
<a id="stdstring-hexifyconst-stdstring-src-const-stdstring-pat"></a>
#####   std::string hexify(const std::string&amp; src, const std::string pat) 
###### *Param:* src 
###### *Param:* pat 
###### *Return:*  
<a id="stdstring-unhexifyconst-stdstring-src"></a>
#####   std::string unhexify(const std::string&amp; src) 
###### *Param:* src 
###### *Return:*  
###### The OS enum 
<a id="os-currentos"></a>
#####  OS currentOS() 
###### *Return:*  
<a id="inline-bool-dofatalconst-stdstring-txt-stdfunctionvoid-f-const-char-file-int-line"></a>
##### inline bool doFatal(const std::string&amp; txt, std::function&lt;void()&gt; f, const char* file, int line) 
###### *Param:* txt 
###### *Param:* f 
###### *Param:* file 
###### *Param:* line 
###### *Return:*  

---

---
##### File Utils 
<a id="stdstring-getlinkconst-stdstring-fname"></a>
#####  std::string getLink(const std::string&amp; fname) 
###### *Param:* fname 
###### *Return:*  
<a id="bool-isdirconst-stdstring-fname"></a>
#####  bool isDir(const std::string&amp; fname) 
###### *Param:* fname 
###### *Return:*  
<a id="stdstring-workingdir"></a>
#####  std::string workingDir() 
###### *Return:*  
<a id="stdstring-abspathconst-stdstring-rpath"></a>
#####  std::string absPath(const std::string&amp; rpath) 
###### *Param:* rpath 
###### *Return:*  
<a id="stdstring-pathpopconst-stdstring-filename"></a>
#####  std::string pathPop(const std::string&amp; filename) 
###### *Param:* filename 
###### *Return:*  
<a id="stdvectorstdtuplestdstring-bool-stdstring-directoryconst-stdstring-dirname"></a>
#####  std::vector&lt;std::tuple&lt;std::string, bool, std::string&gt;&gt; directory(const std::string&amp; dirname) 
###### *Param:* dirname 
###### *Return:*  
<a id="stdstring-tempname"></a>
#####  std::string tempName() 
###### *Return:*  
<a id="stdstring-systemenvconst-stdstring-env"></a>
#####  std::string systemEnv(const std::string&amp; env) 
###### *Param:* env 
###### *Return:*  
<a id="bool-ishiddenentryconst-stdstring-filename"></a>
#####  bool isHiddenEntry(const std::string&amp; filename) 
###### *Param:* filename 
###### *Return:*  
<a id="bool-isexecutableconst-stdstring-filename"></a>
#####  bool isExecutable(const std::string&amp; filename) 
###### *Param:* filename 
###### *Return:*  
<a id="ssize_t-filesizeconst-stdstring-filename"></a>
#####  SSIZE_T fileSize(const std::string&amp; filename) 
###### *Param:* filename 
###### *Return:*  
<a id="bool-renameconst-stdstring-of-const-stdstring-nf"></a>
#####  bool rename(const std::string&amp; of, const std::string&amp; nf) 
###### *Param:* of 
###### *Param:* nf 
###### *Return:*  
<a id="void-removefileconst-stdstring-filename"></a>
#####  void removeFile(const std::string&amp; filename) 
###### *Param:* filename 
<a id="bool-fileexistsconst-stdstring-filename"></a>
#####  bool fileExists(const std::string&amp; filename) 
###### *Param:* filename 
###### *Return:*  
<a id="stdstring-writetotempconst-t-data"></a>
##### std::string writeToTemp(const T&amp; data) 
###### *Param:* data 
###### *Return:*  
<a id="stdvectort-slurpconst-stdstring-file-const-size_t-max-stdnumeric_limitssize_tmax"></a>
##### std::vector&lt;T&gt; slurp(const std::string&amp; file, const size_t max = std::numeric_limits&lt;size_t&gt;::max()) 
###### *Param:* file 
###### *Param:* max 
###### *Return:*  
<a id="stdstring-slurpconst-stdstring-file-const-size_t-max-stdnumeric_limitssize_tmax"></a>
#####  std::string slurp(const std::string&amp; file, const size_t max = std::numeric_limits&lt;size_t&gt;::max()) 
###### *Param:* file 
###### *Param:* max 
###### *Return:*  

---
###### Generated by MarkdownMaker, (c) Markus Mertama 2020 
