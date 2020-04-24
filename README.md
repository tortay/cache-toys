Cache-toys
==========

## Simple tools for checking &amp; influencing filesystem cache content on POSIX systems

While the pagecache tools have historically been developped & tested on some other systems, they are now mostly used/tested on Linux.

Theses tools **no longer work** with Linux kernels 4+.

### drop-from-pagecache
Asks the system to remove files content from the pagecache using `posix_fadvise()`.

### is-in-pagecache
Check if some part of the content of files are in the pagecache.

### prefetch-to-pagecache
Asks the system to prefetch files content to the pagecache using `posix_fadvise()`.

### slices-in-pagecache
Display which parts of the content of files (if any) are in the pagecache.

### hrr
Simple random reader program with optional hints to the pagecache.
Both POSIX (`posix_fadvise()`) and Linux specific (`readahead()`) hints are supported.
