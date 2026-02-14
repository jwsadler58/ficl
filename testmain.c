/*
** stub main for testing FICL under a Posix compliant OS
** $Id: testmain.c,v 1.11 2001-10-28 10:59:19-08 jsadler Exp jsadler $
*/
/*
** Get the latest Ficl release at https://sourceforge.net/projects/ficl/
**
** I am interested in hearing from anyone who uses ficl. If you have
** a problem, a success story, a bug or bugfix, a suggestion, or
** if you would like to contribute to Ficl, please contact me on sourceforge.
**
** L I C E N S E  and  D I S C L A I M E R
**
** Copyright (c) 1997-2026 John W Sadler
** All rights reserved.
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. Neither the name of the copyright holder nor the names of its contributors
**    may be used to endorse or promote products derived from this software
**    without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
** OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
** HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
** SUCH DAMAGE.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#if defined(_WIN32)
    #include <windows.h>
#else
    #include <sys/time.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#if defined(_WIN32)
#include <direct.h>
#define getcwd _getcwd
#define chdir  _chdir
#else
#include <unistd.h>
#endif

/* Line editing support - platform-specific includes */
#if defined(_WIN32)
    #include <conio.h>
    #include <windows.h>
#else
    #include <termios.h>
    #include <sys/ioctl.h>
    #include <signal.h>
#endif

#include "ficl.h"

#if FICL_UNIT_TEST
    #include "dpmath.h"
    #include "unity.h"
#endif

/*
** Line editing constants and data structures
*/
#define MAX_HISTORY 1000
#define INITIAL_LINE_SIZE 256
#define HISTORY_FILE ".ficl_history"

/* Absolute path to history file, set at startup */
static char history_file_path[4096] = "";

/* Key codes for special keys */
#define KEY_NULL 0
#define KEY_CTRL_C 3
#define KEY_CTRL_D 4
#define KEY_BACKSPACE 127
#define KEY_ENTER 13
#define KEY_ESC 27
#define KEY_UP 1000
#define KEY_DOWN 1001
#define KEY_LEFT 1002
#define KEY_RIGHT 1003
#define KEY_HOME 1004
#define KEY_END 1005

/* History management */
static char *history[MAX_HISTORY];
static int history_count = 0;
static int history_pos = 0;

/* Terminal state */
#if defined(_WIN32)
static DWORD orig_console_mode = 0;
static HANDLE h_stdin = NULL;
static HANDLE h_stdout = NULL;
#else
static struct termios orig_termios;
static int raw_mode_enabled = 0;
#endif

/*
** Interrupt support: SIGINT handler calls vmInterrupt() to break
** out of the inner loop without polling.
*/
#if !defined(_WIN32)
static FICL_VM *g_activeVM = NULL;

static void handleSIGINT(int sig)
{
    (void)sig;
    if (g_activeVM && g_activeVM->pState)
        vmInterrupt(g_activeVM);
}

static void enableISIG(FICL_VM *pVM)
{
    g_activeVM = pVM;
    if (raw_mode_enabled)
    {
        struct termios t;
        tcgetattr(STDIN_FILENO, &t);
        t.c_lflag |= ISIG;
        tcsetattr(STDIN_FILENO, TCSANOW, &t);
    }
}

static void disableISIG(void)
{
    g_activeVM = NULL;
    if (raw_mode_enabled)
    {
        struct termios t;
        tcgetattr(STDIN_FILENO, &t);
        t.c_lflag &= ~ISIG;
        tcsetattr(STDIN_FILENO, TCSANOW, &t);
    }
}
#else
    #define enableISIG(pVM) ((void)(pVM))
    #define disableISIG() ((void)0)
#endif

/*
** ======================================================================
** Line Editing Support
** ======================================================================
*/

#if defined(_WIN32)

/* Windows terminal control */
static void enableRawMode(void)
{
    if (h_stdin == NULL) {
        h_stdin = GetStdHandle(STD_INPUT_HANDLE);
        h_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
    }

    if (GetConsoleMode(h_stdin, &orig_console_mode)) {
        DWORD mode = orig_console_mode;
        mode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
        SetConsoleMode(h_stdin, mode);
    }
}

static void disableRawMode(void)
{
    if (h_stdin && orig_console_mode) {
        SetConsoleMode(h_stdin, orig_console_mode);
    }
}

static int readKey(void)
{
    INPUT_RECORD rec;
    DWORD count;

    while (1) {
        if (!ReadConsoleInput(h_stdin, &rec, 1, &count) || count == 0) {
            return KEY_NULL;
        }

        if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.bKeyDown) {
            KEY_EVENT_RECORD *key = &rec.Event.KeyEvent;

            /* Handle special keys */
            switch (key->wVirtualKeyCode) {
                case VK_LEFT: return KEY_LEFT;
                case VK_RIGHT: return KEY_RIGHT;
                case VK_UP: return KEY_UP;
                case VK_DOWN: return KEY_DOWN;
                case VK_HOME: return KEY_HOME;
                case VK_END: return KEY_END;
                case VK_BACK: return KEY_BACKSPACE;
                case VK_RETURN: return KEY_ENTER;
                default:
                    /* Return ASCII character if printable */
                    if (key->uChar.AsciiChar) {
                        return key->uChar.AsciiChar;
                    }
            }
        }
    }
}

#else

/* Unix/macOS terminal control using termios */
static void enableRawMode(void)
{
    struct termios raw;

    if (!isatty(STDIN_FILENO)) {
        return;  /* Not a terminal */
    }

    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
        return;
    }

    raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);  /* Disable echo, canonical mode, and signals */
    raw.c_cc[VMIN] = 1;   /* Read at least 1 character */
    raw.c_cc[VTIME] = 0;  /* No timeout */

    /* Use TCSADRAIN to preserve any pending input when entering raw mode */
    if (tcsetattr(STDIN_FILENO, TCSADRAIN, &raw) == -1) {
        return;
    }

    raw_mode_enabled = 1;
}

static void disableRawMode(void)
{
    if (raw_mode_enabled) {
        /* Use TCSADRAIN instead of TCSAFLUSH to preserve pending input (for paste) */
        tcsetattr(STDIN_FILENO, TCSADRAIN, &orig_termios);
        raw_mode_enabled = 0;
    }
}

static int readKey(void)
{
    int nread;
    char c;
    char seq[3];

    nread = read(STDIN_FILENO, &c, 1);
    if (nread != 1) {
        return KEY_NULL;
    }

    /* Handle escape sequences */
    if (c == KEY_ESC) {
        /* Read the next two bytes for escape sequences */
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return KEY_ESC;
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return KEY_ESC;

        if (seq[0] == '[') {
            switch (seq[1]) {
                case 'A': return KEY_UP;
                case 'B': return KEY_DOWN;
                case 'C': return KEY_RIGHT;
                case 'D': return KEY_LEFT;
                case 'H': return KEY_HOME;
                case 'F': return KEY_END;
                case '1':
                case '4':
                    /* Home (ESC[1~) or End (ESC[4~) - read the ~ */
                    if (read(STDIN_FILENO, &seq[2], 1) == 1 && seq[2] == '~') {
                        return (seq[1] == '1') ? KEY_HOME : KEY_END;
                    }
                    break;
            }
        }
        return KEY_ESC;
    }

    /* Handle backspace (could be 127 or 8) */
    if (c == 127 || c == 8) {
        return KEY_BACKSPACE;
    }

    /* Handle Ctrl+C and Ctrl+D */
    if (c == KEY_CTRL_C || c == KEY_CTRL_D) {
        return c;
    }

    /* Handle Enter (could be \r or \n) */
    if (c == '\r' || c == '\n') {
        return KEY_ENTER;
    }

    return (unsigned char)c;
}

#endif

/*
** Get the terminal width in columns
*/
static int getTerminalWidth(void)
{
#if defined(_WIN32)
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        return csbi.dwSize.X;
    }
#else
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) {
        return ws.ws_col;
    }
#endif
    return 80;  /* default fallback */
}

/*
** Refresh the current line on screen
** Uses horizontal scrolling when the line exceeds the terminal width
** Cursor positioning: prompt + cursor position
*/
static void refreshLine(const char *prompt, const char *buf, int len, int cursor)
{
    int plen = strlen(prompt);
    int cols = getTerminalWidth();
    int available = cols - plen;  /* space for buffer text after prompt */
    int scroll_offset = 0;
    int visible_len;
    int cursor_col;

    if (available < 1) available = 1;

    /* Calculate scroll offset to keep cursor visible */
    if (cursor >= available) {
        scroll_offset = cursor - available + 1;
    }

    visible_len = len - scroll_offset;
    if (visible_len > available) {
        visible_len = available;
    }
    if (visible_len < 0) visible_len = 0;

    cursor_col = plen + (cursor - scroll_offset);

#if defined(_WIN32)
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    COORD coord;

    if (GetConsoleScreenBufferInfo(h_stdout, &csbi)) {
        coord.X = 0;
        coord.Y = csbi.dwCursorPosition.Y;
        SetConsoleCursorPosition(h_stdout, coord);

        DWORD written;
        FillConsoleOutputCharacter(h_stdout, ' ', csbi.dwSize.X, coord, &written);

        printf("%s%.*s", prompt, visible_len, buf + scroll_offset);

        coord.X = cursor_col;
        SetConsoleCursorPosition(h_stdout, coord);
        fflush(stdout);
    }
#else
    /* Move cursor to start of line and clear line */
    write(STDOUT_FILENO, "\r\x1b[K", 4);

    /* Write prompt */
    if (plen > 0) {
        write(STDOUT_FILENO, prompt, plen);
    }

    /* Write visible portion of buffer */
    if (visible_len > 0) {
        write(STDOUT_FILENO, buf + scroll_offset, visible_len);
    }

    /* Move cursor to correct position */
    {
        char cursorSeq[16];
        int seqLen = snprintf(cursorSeq, sizeof(cursorSeq), "\x1b[%dG", cursor_col + 1);
        write(STDOUT_FILENO, cursorSeq, seqLen);
    }
#endif
}

/*
** History management functions
*/
static void addHistory(const char *line)
{
    char *copy;

    if (!line || line[0] == '\0') {
        return;  /* Don't add empty lines */
    }

    /* Don't add if it's the same as the last entry */
    if (history_count > 0 && strcmp(history[history_count - 1], line) == 0) {
        return;
    }

    copy = strdup(line);
    if (!copy) {
        return;
    }

    if (history_count < MAX_HISTORY) {
        history[history_count++] = copy;
    } else {
        /* History full - remove oldest entry */
        free(history[0]);
        memmove(history, history + 1, sizeof(char *) * (MAX_HISTORY - 1));
        history[MAX_HISTORY - 1] = copy;
    }
}

static void loadHistory(void)
{
    FILE *fp;
    char buf[4096];

    fp = fopen(history_file_path, "r");
    if (!fp) {
        return;  /* File doesn't exist yet, that's OK */
    }

    while (fgets(buf, sizeof(buf), fp)) {
        int len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') {
            buf[len - 1] = '\0';  /* Remove newline */
        }
        addHistory(buf);
    }

    fclose(fp);
}

static void saveHistoryLine(const char *line)
{
    FILE *fp;

    if (!line || line[0] == '\0') {
        return;
    }

    fp = fopen(history_file_path, "a");
    if (fp) {
        fputs(line, fp);
        fputc('\n', fp);
        fclose(fp);
    }
}

/*
** Initialize the absolute path to the history file.
** Places history in the user's home directory (~/.ficl_history).
*/
static void initHistoryPath(void)
{
    const char *home;

    home = getenv("HOME");
    if (!home)
        home = getenv("USERPROFILE");  /* Windows fallback */

    if (home && strlen(home) + 1 + strlen(HISTORY_FILE) + 1 <= sizeof(history_file_path)) {
        snprintf(history_file_path, sizeof(history_file_path), "%s/%s", home, HISTORY_FILE);
    } else {
        strncpy(history_file_path, HISTORY_FILE, sizeof(history_file_path) - 1);
        history_file_path[sizeof(history_file_path) - 1] = '\0';
    }
}

/*
** Check if there's input available without blocking (for paste detection)
*/
static int inputAvailable(void)
{
#if defined(_WIN32)
    DWORD numEvents = 0;
    if (GetNumberOfConsoleInputEvents(GetStdHandle(STD_INPUT_HANDLE), &numEvents)) {
        return numEvents > 1;  /* More than just our current read */
    }
    return 0;
#else
    /* Use select() with zero timeout to check for pending input */
    struct timeval tv;
    fd_set readfds;

    tv.tv_sec = 0;
    tv.tv_usec = 0;

    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    return select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv) > 0;
#endif
}

/*
** Simple line reading fallback for non-TTY input (pipes, redirects, pastes)
*/
static char* readLineSimple(const char *prompt)
{
    char buf[4096];

    printf("%s", prompt);
    fflush(stdout);

    if (fgets(buf, sizeof(buf), stdin) == NULL) {
        return NULL;
    }

    /* Remove newline if present */
    int len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    }

    return strdup(buf);
}

/*
** Main line editing function
** Returns a malloc'd string or NULL on EOF
*/
static char* editLine(const char *prompt)
{
    char *buf;
    int buf_capacity = INITIAL_LINE_SIZE;
    int buf_len = 0;
    int cursor = 0;
    int history_index = history_count;
    char *saved_line = NULL;  /* Save current line when navigating history */

#if defined(_WIN32)
    /* Check if stdin is a console on Windows */
    DWORD mode;
    if (!GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode)) {
        return readLineSimple(prompt);  /* Not a console */
    }
#else
    /* Check if stdin is a TTY on Unix/macOS */
    if (!isatty(STDIN_FILENO)) {
        return readLineSimple(prompt);  /* Not a TTY */
    }
#endif

    /* Enable raw mode for the terminal */
    enableRawMode();

    /* Allocate initial buffer */
    buf = malloc(buf_capacity);
    if (!buf) {
        return NULL;
    }
    buf[0] = '\0';

    /* Start on a new line to avoid clearing previous output */
    printf("\n");

    /* Print initial prompt */
    printf("%s", prompt);
    fflush(stdout);

    while (1) {
        int key = readKey();

        if (key == KEY_NULL) {
            free(buf);
            if (saved_line) free(saved_line);
            disableRawMode();
            return NULL;
        }

        if (key == KEY_CTRL_C || key == KEY_CTRL_D) {
            /* Exit */
            printf("\n");
            free(buf);
            if (saved_line) free(saved_line);
            disableRawMode();
            return NULL;
        }

        if (key == KEY_ENTER) {
            /* Submit line */
            printf("\n");
            if (saved_line) free(saved_line);
            /* Disable raw mode before returning so inputAvailable() works correctly */
            disableRawMode();
            return buf;
        }

        if (key == KEY_BACKSPACE) {
            /* Delete character before cursor */
            if (cursor > 0) {
                memmove(buf + cursor - 1, buf + cursor, buf_len - cursor + 1);
                cursor--;
                buf_len--;
                refreshLine(prompt, buf, buf_len, cursor);
            }
            continue;
        }

        if (key == KEY_LEFT) {
            /* Move cursor left */
            if (cursor > 0) {
                cursor--;
                refreshLine(prompt, buf, buf_len, cursor);
            }
            continue;
        }

        if (key == KEY_RIGHT) {
            /* Move cursor right */
            if (cursor < buf_len) {
                cursor++;
                refreshLine(prompt, buf, buf_len, cursor);
            }
            continue;
        }

        if (key == KEY_HOME) {
            /* Move to start of line */
            cursor = 0;
            refreshLine(prompt, buf, buf_len, cursor);
            continue;
        }

        if (key == KEY_END) {
            /* Move to end of line */
            cursor = buf_len;
            refreshLine(prompt, buf, buf_len, cursor);
            continue;
        }

        if (key == KEY_UP) {
            /* Navigate to previous history entry */
            if (history_index > 0) {
                /* Save current line if at the end of history */
                if (history_index == history_count && !saved_line) {
                    saved_line = strdup(buf);
                }

                history_index--;
                strcpy(buf, history[history_index]);
                buf_len = strlen(buf);
                cursor = buf_len;
                refreshLine(prompt, buf, buf_len, cursor);
            }
            continue;
        }

        if (key == KEY_DOWN) {
            /* Navigate to next history entry */
            if (history_index < history_count) {
                history_index++;
                if (history_index == history_count) {
                    /* Restore saved line */
                    if (saved_line) {
                        strcpy(buf, saved_line);
                        free(saved_line);
                        saved_line = NULL;
                    } else {
                        buf[0] = '\0';
                    }
                } else {
                    strcpy(buf, history[history_index]);
                }
                buf_len = strlen(buf);
                cursor = buf_len;
                refreshLine(prompt, buf, buf_len, cursor);
            }
            continue;
        }

        /* Regular character - insert at cursor */
        if (key >= 32 && key < 127) {
            /* Grow buffer if needed */
            if (buf_len + 1 >= buf_capacity) {
                buf_capacity *= 2;
                char *new_buf = realloc(buf, buf_capacity);
                if (!new_buf) {
                    free(buf);
                    if (saved_line) free(saved_line);
                    return NULL;
                }
                buf = new_buf;
            }

            /* Insert character */
            memmove(buf + cursor + 1, buf + cursor, buf_len - cursor + 1);
            buf[cursor] = (char)key;
            cursor++;
            buf_len++;
            refreshLine(prompt, buf, buf_len, cursor);
        }
    }
}

/*
** ======================================================================
** End of Line Editing Support
** ======================================================================
*/

/*
** Ficl interface to POSIX getcwd()
** Prints the current working directory using the VM's
** textOut method...
*/
static void ficlGetCWD(FICL_VM *pVM)
{
    char *cp;

   cp = getcwd(NULL, 80);
    vmTextOut(pVM, cp, 1);
    free(cp);
    return;
}

/*
** Ficl interface to chdir
** Gets a newline (or NULL) delimited string from the input
** and feeds it to the Posix chdir function...
** Example:
**    cd c:\tmp
*/
static void ficlChDir(FICL_VM *pVM)
{
    FICL_STRING *pFS = (FICL_STRING *)pVM->pad;
    vmGetString(pVM, pFS, '\n');
    if (pFS->count > 0)
    {
       int err = chdir(pFS->text);
       if (err)
        {
            vmTextOut(pVM, "Error: path not found", 0);
            vmTextOut(pVM, pFS->text, 1);
            vmThrow(pVM, VM_ERREXIT);
        }
    }
    else
    {
        vmTextOut(pVM, "Warning (chdir): nothing happened", 1);
    }
    return;
}

/*
** Ficl interface to system (ANSI)
** Gets a newline (or NULL) delimited string from the input
** and feeds it to the Posix system function...
** Example:
**    system ls -l
*/
static void ficlSystem(FICL_VM *pVM)
{
    FICL_STRING *pFS = (FICL_STRING *)pVM->pad;

    vmGetString(pVM, pFS, '\n');
    if (pFS->count > 0)
    {
        int err = system(pFS->text);
        if (err)
        {
            snprintf(pVM->pad, sizeof(pVM->pad), "System call returned %d", err);
            vmTextOut(pVM, pVM->pad, 1);
            vmThrow(pVM, VM_QUIT);
        }
    }
    else
    {
        vmTextOut(pVM, "Warning (system): nothing happened", 1);
    }
    return;
}

/*
** Load a text file and execute it
** Line oriented... filename is newline (or NULL) delimited.
** Example:
**    load test.ficl
*/
#define nLINEBUF 256
static void ficlLoad(FICL_VM *pVM)
{
    char    cp[nLINEBUF];
    char    filename[nLINEBUF];
    FICL_STRING *pFilename = (FICL_STRING *)filename;
    int     nLine = 0;
    FILE   *fp;
    int     result;
    CELL    id;
    struct stat buf;

    vmGetString(pVM, pFilename, '\n');

    if (pFilename->count <= 0)
    {
        vmTextOut(pVM, "Warning (load): empty filename", 1);
         return;
    }

    /*
    ** get the file's size and make sure it exists
    */
    result = stat( pFilename->text, &buf );

    if (result != 0)
    {
        vmTextOut(pVM, "Unable to stat file: ", 0);
        vmTextOut(pVM, pFilename->text, 1);
        vmThrow(pVM, VM_ERREXIT);
    }

    fp = fopen(pFilename->text, "r");
    if (!fp)
    {
        vmTextOut(pVM, "Unable to open file ", 0);
        vmTextOut(pVM, pFilename->text, 1);
        vmThrow(pVM, VM_ERREXIT);
    }

    vmTextOut(pVM, "Loading: ", 0);
    vmTextOut(pVM, pFilename->text, 1);

    /* save any prior file in process*/
    id = pVM->sourceID;
    pVM->sourceID.p = (void *)fp;

    /* feed each line to ficlExec */
    while (fgets(cp, nLINEBUF, fp))
    {
        int len = strlen(cp) - 1;

        nLine++;
        if (len <= 0)
            continue;

        if (cp[len] == '\n')
            cp[len] = '\0';

        result = ficlExec(pVM, cp);
        /* handle "bye" in loaded files. --lch */
        switch (result)
        {
            case VM_OUTOFTEXT:
            case VM_USEREXIT:
                break;

            default:
                pVM->sourceID = id;
                fclose(fp);
                vmThrowErr(pVM, "Error loading file <%s> line %d", pFilename->text, nLine);
                break;
        }
    }
    /*
    ** Pass an empty line with SOURCE-ID == -1 to flush
    ** any pending REFILLs (as required by FILE wordset)
    */
    pVM->sourceID.i = -1;
    ficlExec(pVM, "");

    pVM->sourceID = id;
    fclose(fp);

    /* handle "bye" in loaded files. --lch */
    if (result == VM_USEREXIT)
        vmThrow(pVM, VM_USEREXIT);
    return;
}

/*
** Dump a tab delimited file that summarizes the contents of the
** dictionary hash table by hashcode...
*/
static void spewHash(FICL_VM *pVM)
{
    FICL_HASH *pHash = vmGetDict(pVM)->pForthWords;
    FICL_WORD *pFW;
    FILE *pOut;
    unsigned i;
    unsigned nHash = pHash->size;

    if (!vmGetWordToPad(pVM))
        vmThrow(pVM, VM_OUTOFTEXT);

    pOut = fopen(pVM->pad, "w");
    if (!pOut)
    {
        vmTextOut(pVM, "unable to open file", 1);
        return;
    }

    /* header line */
    if (fputs("Row\tnEntries\tNames\n", pOut) < 0) {
        fclose(pOut);
        return;
    }

    for (i=0; i < nHash; i++)
    {
        int n = 0;
        char buf[512];
        int len;

        pFW = pHash->table[i];
        while (pFW)
        {
            n++;
            pFW = pFW->link;
        }

        len = snprintf(buf, sizeof(buf), "%d\t%d", i, n);
        if (len < 0 || len >= (int)sizeof(buf) || fputs(buf, pOut) < 0)
        {
            fclose(pOut);
            return;
        }

        pFW = pHash->table[i];
        while (pFW)
        {
            len = snprintf(buf, sizeof(buf), "\t%.32s", pFW->name);
            if (len < 0 || len >= (int)sizeof(buf) || fputs(buf, pOut) < 0)
            {
                fclose(pOut);
                return;
            }
            pFW = pFW->link;
        }

        if (fputc('\n', pOut) < 0)
        {
            fclose(pOut);
            return;
        }
    }

    fclose(pOut);
    return;
}

static void ficlBreak(FICL_VM *pVM)
{
    pVM->state = pVM->state; /* no-op for the debugger to grab - set a breakpoint here */
    return;
}

static void ficlClock(FICL_VM *pVM)
{
    FICL_UNS now;
#if defined(_WIN32)
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    now = (FICL_UNS)counter.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    now = (FICL_UNS)ts.tv_sec * 1000000000u + (FICL_UNS)ts.tv_nsec;
#endif
    stackPushUNS(pVM->pStack, now);
    return;
}

static void clocksPerSec(FICL_VM *pVM)
{
#if defined(_WIN32)
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    stackPushUNS(pVM->pStack, (FICL_UNS)freq.QuadPart);
#else
    stackPushUNS(pVM->pStack, 1000000000u);
#endif
    return;
}

/*
**             t e s t - e r r o r
** Test error reporting callback for scripted tests (see test/tester.fr)
*/
static int nTestFails = 0;
static void testError(FICL_VM *pVM)
{
    nTestFails++;
    return;
}

static void nTestErrors(FICL_VM *pVM)
{
    stackPushINT(pVM->pStack, nTestFails);
    return;
}


void buildTestInterface(FICL_SYSTEM *pSys)
{
    ficlBuild(pSys, "break",    ficlBreak,    FW_DEFAULT);
    ficlBuild(pSys, "clock",    ficlClock,    FW_DEFAULT);
    ficlBuild(pSys, "cd",       ficlChDir,    FW_DEFAULT);
    ficlBuild(pSys, "load",     ficlLoad,     FW_DEFAULT);
    ficlBuild(pSys, "pwd",      ficlGetCWD,   FW_DEFAULT);
    ficlBuild(pSys, "system",   ficlSystem,   FW_DEFAULT);
    ficlBuild(pSys, "spewhash", spewHash,     FW_DEFAULT);
    ficlBuild(pSys, "test-error", testError,  FW_DEFAULT); /* ficltest.fr signaling */
    ficlBuild(pSys, "#errors",  nTestErrors,  FW_DEFAULT);
    ficlBuild(pSys, "clocks/sec",
                                clocksPerSec, FW_DEFAULT);

    return;
}

#if FICL_UNIT_TEST
    void setUp(void)
    {
    }

    void tearDown(void)
    {
    }

    static void dummyCode(FICL_VM *pVM)
    {
        (void)pVM;
    }

    static void wordLayoutTest(void)
    {
        TEST_ASSERT_TRUE((FICL_WORD_HEADER_BYTES % sizeof(CELL)) == 0);
        TEST_ASSERT_TRUE(FICL_WORD_BASE_BYTES == FICL_WORD_HEADER_BYTES + sizeof(CELL));
        TEST_ASSERT_TRUE(FICL_WORD_BASE_CELLS == (FICL_WORD_BASE_BYTES / sizeof(CELL)));
    }

    static void wordAppendBodyTest(void)
    {
        FICL_DICT *dp = dictCreate(256);
        STRINGINFO si;
        FICL_WORD *pFW;
        const char *name = "testword";

        SI_SETLEN(si, strlen(name));
        SI_SETPTR(si, (char *)name);
        pFW = dictAppendWord2(dp, si, dummyCode, FW_DEFAULT);

        TEST_ASSERT_TRUE(pFW != NULL);
        TEST_ASSERT_TRUE(pFW->param == dp->here);
        TEST_ASSERT_TRUE(pFW->nName == (FICL_COUNT)strlen(name));
        {
            CELL *body = pFW->param + 1;
            FICL_WORD *from = (FICL_WORD *)((char *)body - FICL_WORD_BASE_BYTES);
            TEST_ASSERT_TRUE(from == pFW);
        }

        dictDelete(dp);
    }

    static void hashLayoutTest(void)
    {
        size_t base = offsetof(FICL_HASH, table);
        TEST_ASSERT_TRUE((base % sizeof(void *)) == 0);
        TEST_ASSERT_TRUE(FICL_HASH_BYTES(1) == base + sizeof(FICL_WORD *));
        TEST_ASSERT_TRUE(FICL_HASH_BYTES(4) == base + 4 * sizeof(FICL_WORD *));
    }

    static void hashCreateTest(void)
    {
        FICL_DICT *dp = dictCreateHashed(256, 7);
        FICL_HASH *pHash = dp->pForthWords;
        unsigned i;

        TEST_ASSERT_TRUE(pHash != NULL);
        TEST_ASSERT_TRUE(pHash->size == 7);
        for (i = 0; i < pHash->size; ++i)
        {
            TEST_ASSERT_TRUE(pHash->table[i] == NULL);
        }

        dictDelete(dp);
    }

#endif

#define nINBUF 256

int main(int argc, char **argv)
{
    int ret = 0;
    char in[nINBUF];
    FICL_VM *pVM;
    FICL_SYSTEM *pSys;
    int i;
    int fUnit = 0;

    /* Find '--test' anywhere on the command line */
    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--test") == 0)
        {
#if FICL_UNIT_TEST
            fUnit = 1;
            break;
#else
            printf("Error: Unit tests not enabled in this build.\n");
            return 1;
#endif
        }
    }

#if FICL_UNIT_TEST
    if (fUnit)
    {
        UNITY_BEGIN();
        RUN_TEST(dpmUnitTest);
        RUN_TEST(wordLayoutTest);
        RUN_TEST(wordAppendBodyTest);
        RUN_TEST(hashLayoutTest);
        RUN_TEST(hashCreateTest);
        nTestFails = UNITY_END();
        if (nTestFails > 0)
        {
            printf("***** Unit tests failed: %d *****\n", nTestFails);
            return nTestFails;
        }
    }
#endif

    pSys = ficlInitSystem(20000);
    buildTestInterface(pSys);
    pVM = ficlNewVM(pSys);

    ret = ficlEvaluate(pVM, ".ver 2 spaces .( " __DATE__ " ) cr");
    if (fUnit)                 /* run tests and return rather than going interactive */
    {
        ret = ficlEvaluate(pVM, "cd test\n load ficltest.fr");
        ficlTermSystem(pSys);
        if (ret == VM_ERREXIT)
            return 1;

        if (nTestFails > 0)
            printf("***** Scripted tests failed: %d *****\n", nTestFails);
        else
            printf("***** Scripted tests passed *****\n");

        return nTestFails;
    }

    /* Install SIGINT handler for Ctrl+C interrupt support */
#if !defined(_WIN32)
    {
        struct sigaction sa;
        sa.sa_handler = handleSIGINT;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;  /* no SA_RESTART — allow reads to be interrupted */
        sigaction(SIGINT, &sa, NULL);
    }
#endif

    /* Initialize absolute path to history file */
    initHistoryPath();

    /* Load command history from file */
    loadHistory();

    /* Main REPL with line editing */
    char *line;
    int use_simple_mode = 0;  /* Use simple mode when pasting */

    while (ret != VM_USEREXIT)
    {
        /* Check if we should use simple mode (for pasted content) */
        if (use_simple_mode || inputAvailable())
        {
            line = readLineSimple("");
            use_simple_mode = 1;  /* Keep using simple mode while input is available */
        } else
        {
            line = editLine("");
            use_simple_mode = 0;
        }

        if (!line) break;

        if (line[0] != '\0')
        {
            enableISIG(pVM);
            ret = ficlExec(pVM, line);
            disableISIG();
            addHistory(line);
            saveHistoryLine(line);
        }
        free(line);

        /* If no more input available, go back to interactive mode */
        if (use_simple_mode && !inputAvailable())
        {
            use_simple_mode = 0;
        }
    }

    /* Restore terminal mode before exit */
    disableRawMode();

    ficlTermSystem(pSys);
    return 0;
}

