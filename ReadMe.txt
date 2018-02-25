
cblib notes :

USAGE :

** !!! first of all you must always include "cblib/Base.h" first before anything

You can just use "linklib.h" in your client app to bring in the libraries.
(though I did stick absolute paths in there so you may need to change that)

BmpImageJpeg uses the "jpeg-6b" library, so if you don't have that you can just turn it off in BmpImageJpeg.cpp

--------------------------------------------------

Everything in cblib is namespaced in "cb:" to avoid link conflicts.  To use cb code you can either call it explicitly like :

NS_CB::Timer::GetSeconds()

or you put a "USE_CB" declaration at the top of your file.

To write code in cb namespace use START_CB , END_CB ; don't use "cb:" directly since that may change to avoid conflicts.

Note that I play really loose with my #defines so there may well be conflicts there.  My preferred way to handle that is to #undef
other people's #defines that conflict with mine, but it can get a little ugly.  I'm not really happy with having to use CB_MIN names
everywhere.

--------------------------------------------------

There are various levels of drinking the koolaid in "cblib" .

You can just call some functions and do okay.

If you buy into my style of STL usage, the you need to make sure to be using STLport and letting my allocator replacement work.

The big STLport hammer that I do is in "_alloc.h" I change their calls to malloc to call "StlAlloc".  That lets me patch the allocator
for all ops at the bottom level.  Then anywhere you use STL stuff make sure you include "stl_basics" before anything else.

The next level of buying in is getting on the smart pointer train.  
	See http://www.cbloom.com/3d/techdocs/smart_pointers.txt

The next level of buying in would be do the Reflection thing and IO() members.

--------------------------------------------------

cblib todos :

1. a simple ResMgr that just does res caching by name
	you call like ResMgr::Get<File>("name")

2. simple PROFILE from Galaxy with tsc

3. Some of the basic old file util stuff from chuksh/crblib like exists/etc
	really I should rewrite my old dir-walker and pattern matcher
	and get all my old console apps over

the bmp/jpg stuff has some problems
	standardize y up
	standardize bgr/rgb
	don't promote 24->32
	
