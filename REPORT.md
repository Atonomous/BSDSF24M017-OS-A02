# Operating Systems Assignment 02 - Technical Report
**Student Roll Number:** BSDSF24M017
**Student Name:** Mazhar Shahzad Mughal
**Course:** Operating Systems
**Instructor:** Dr. Muhammad Arif But

---

## Feature 2: Long Listing Format (-l)

### Q1: What is the crucial difference between the stat() and lstat() system calls? In the context of the ls command, when is it more appropriate to use lstat()?
- **Crucial Difference:**
  Both `stat()` and `lstat()` populate a `struct stat` containing metadata for a target path. The distinction occurs when operating on a **symbolic link**:
  - `stat()` dereferences (follows) the symbolic link and retrieves the metadata of the destination file the link points to.
  - `lstat()` does **not** follow the symbolic link; it retrieves the metadata of the symbolic link file itself (including its own link size, permissions, and `st_mode` indicating `S_IFLNK`).
- **Context in `ls`:**
  In the `ls` command, `lstat()` is the appropriate choice when inspecting entries in a directory. A directory listing must report the true attributes of the link itself (e.g., displaying `l` as the first permission character and recognizing it as a soft link) rather than masking it as its destination.

### Q2: The st_mode field in struct stat is an integer that contains both the file type and the permission bits. Explain how you can use bitwise operators and predefined macros to extract this information.
- **Structure of `st_mode`:**
  `st_mode` is a 16-bit integer composed of two major bitfields:
  - **Upper 4 bits (bits 12-15):** File type. Extracted using macros `S_ISREG()`, `S_ISDIR()`, `S_ISLNK()`, `S_ISCHR()`, `S_ISBLK()`, `S_ISFIFO()`, `S_ISSOCK()`. These macros internally apply a bitmask (`st_mode & S_IFMT`) and compare the result against type constants.
  - **Lower 12 bits (bits 0-11):** Permission bits (rwx for owner, group, others) and special bits (SUID, SGID, Sticky). Extracted using bitwise AND with predefined constants: `S_IRUSR`, `S_IWUSR`, `S_IXUSR`, ..., `S_ISUID`, `S_ISGID`, `S_ISVTX`.
- **Example:**
  ```c
  if (S_ISDIR(st.st_mode))  // checks file type
  if (st.st_mode & S_IRUSR) // checks owner read permission
  ```

---

## Feature 3: Column Display "Down Then Across"

### Q1: Using the formula index = c * num_rows + r, explain why a simple sequential loop (for i = 0 to N-1) is not sufficient for printing directory entries in column-major order.
- **Explanation:**
  Column-major order means items are arranged vertically first: Row 0 holds items {0, num_rows, 2*num_rows, ...}, Row 1 holds {1, num_rows+1, 2*num_rows+1, ...}, etc.
  A simple sequential loop `for i = 0..N-1` visits items in row-major order: {0, 1, 2, ...}. The mapping between the screen position (r, c) and the data index is:
  index = c * num_rows + r
- **Why a Single Loop is Insufficient:**
  Standard terminal emulators are stream-based. Once a line is terminated with `\n`, the cursor cannot move backwards upward to append text to earlier lines. Consequently, an outer loop must iterate through rows (0 ... num_rows-1) and an inner loop must jump across columns by increments of num_rows to render each line in order.

### Q2: What is the purpose of the ioctl system call in this context? What would be the limitations of your program if you only used a fixed-width fallback (e.g., 80 columns)?
- **Purpose of `ioctl`:**
  `ioctl(STDOUT_FILENO, TIOCGWINSZ, &w)` queries the terminal driver for the current screen geometry (`w.ws_col`). This allows the layout engine to dynamically adjust column count when the terminal window is resized.
- **Limitations of Fixed 80-Column Fallback:**
  - Wastes space on widescreen/modern terminals (160+ columns).
  - Causes overflow and line-wrapping distortion on narrow terminals (e.g., side-by-side terminal splits of 40-60 columns).

---

## Feature 4: Horizontal Column Display (-x Option)

### Q1: Compare the implementation complexity of the "down then across" (vertical) printing logic versus the "across" (horizontal) printing logic. Which one requires more pre-calculation and why?
- **Comparison:**
  - **Horizontal (`-x`)** printing is row-major. Items are processed sequentially in a single pass from index 0 to N-1, tracking the horizontal cursor position and outputting `\n` whenever an entry exceeds the terminal width.
  - **Down-then-across (vertical)** requires computing total rows num_rows and non-linear indexing (c * num_rows + r) before any character can be printed.
- **Conclusion:**
  "Down then across" requires significantly more pre-calculation because row allocation depends on the total file count.

### Q2: Describe the strategy you used in your code to manage the different display modes (-l, -x, and default). How did your program decide which function to call for printing?
- An `enum DisplayMode { MODE_VERTICAL, MODE_LONG, MODE_HORIZONTAL }` variable (`display_mode`) maintains state.
- During command-line argument parsing with `getopt()`, flag `-l` assigns `MODE_LONG` and flag `-x` assigns `MODE_HORIZONTAL`.
- After directory entries are collected and sorted, a `switch (display_mode)` executes the corresponding renderer (`print_long_entry`, `print_horizontal`, or `print_down_then_across`).

---

## Feature 5: Alphabetical Sort

### Q1: Why is it necessary to read all directory entries into memory before you can sort them? What are the potential drawbacks of this approach for directories containing millions of files?
- **Necessity:**
  `readdir()` returns entries in physical filesystem order (inode or hash-table order on disk), which is not sorted. In-memory comparison-based sorting algorithms (like Quicksort) require random access across all elements.
- **Drawbacks for Millions of Files:**
  1. **Memory Exhaustion:** Storing millions of string pointers and heap allocations creates substantial RAM overhead and may trigger Out-Of-Memory (OOM) termination.
  2. **High Latency:** Significant delay occurs before the user sees the first file output while the system reads and sorts millions of records.

### Q2: Explain the purpose and signature of the comparison function required by qsort(). How does it work, and why must it take const void * arguments?
- **Signature:** `int cmp(const void *a, const void *b);`
- **Purpose & Generality:**
  `qsort()` is a generic sorting utility that does not know the data type of the elements being sorted. Passing `const void *` allows generic pointers to elements of any size.
- **How it Works:**
  For an array of strings (`char **`), each element is a pointer (`char *`). Thus, `a` and `b` point to `char *`. Inside the comparator, we cast `(const char **)a`, dereference to obtain `const char *`, and pass them to `strcmp()`.

---

## Feature 6: Colorized Output

### Q1: How do ANSI escape codes work to produce color in a standard Linux terminal? Show the specific code sequence for printing text in green.
- **Mechanism:**
  ANSI escape sequences are in-band control sequences starting with the ESC character (`\033` or `\x1b`), followed by `[` (Control Sequence Introducer, CSI), numerical parameters, and character `m` (Select Graphic Rendition). Terminal emulators interpret these byte sequences to alter foreground/background color instead of printing raw characters.
- **Green Code Sequence:**
  - Foreground Green: `\033[0;32m` (Normal) or `\033[1;32m` (Bold green).
  - Reset to default: `\033[0m`.
  - Example: `printf("\033[1;32m%s\033[0m", filename);`

### Q2: To color an executable file, you need to check its permission bits. Explain which bits in the st_mode field you need to check to determine if a file is executable by the owner, group, or others.
- The three executable bits in `st_mode` are:
  - `S_IXUSR` (`00100` octal): Owner execute
  - `S_IXGRP` (`00010` octal): Group execute
  - `S_IXOTH` (`00001` octal): Others execute
- A file is executable if any of these bits are set:
  ```c
  if (st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
      // File is executable -> Green
  }
  ```

---

## Feature 7: Recursive Listing (-R)

### Q1: In a recursive function, what is a "base case"? In the context of your recursive ls, what is the base case that stops the recursion from continuing forever?
- **Base Case:**
  A terminating condition where the recursive function does not invoke itself further and returns control up the call stack.
- **Base Case in `ls -R`:**
  1. A directory has no subdirectories (all entries are regular files).
  2. The special directory entries `.` and `..` are filtered out. (Without filtering, recursion would loop infinitely between parent and child).
  3. `opendir()` fails (e.g., permission denied), stopping descent into restricted trees.

### Q2: Explain why it is essential to construct a full path before making a recursive call. What would happen if you simply called do_ls("subdir") from within the do_ls("parent_dir") function call?
- **Reason:**
  A process has a single working directory (CWD). Directory entries returned by `readdir()` are relative to the directory currently being read, not relative to the program's working directory.
- **What would happen:**
  If `do_ls("parent_dir")` calls `do_ls("subdir")`, the operating system looks for `./subdir` in the process CWD instead of `./parent_dir/subdir`. If `./subdir` does not exist in the root CWD, `opendir()` fails with `ENOENT` (No such file or directory) and traversal terminates.
```

---


