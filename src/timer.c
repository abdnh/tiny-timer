#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iup.h>

#ifdef _WIN32
#include <Windows.h>

#elif defined(__linux__)
#include <pthread.h>

#endif

struct timer {
    int current;
    int to;
    int up;  // (to - current >= 0) ? 1 : 0; /* increment or decrement ? */
};

void sec_to_hms(int sec, int *arr) {
    /* convert 'sec' into hours, minutes, and seconds and store them in 'arr'.
    Useful for displaying time in the format h:m:s */

    arr[0] = sec / 3600; /* hours */
    sec -= arr[0] * 3600;
    arr[1] = (sec / 60); /* minutes */
    sec -= arr[1] * 60;
    arr[2] = sec; /* seconds */
}

static int hms_to_sec(char *str) {
    /* convert 'str' to seconds ASSUMING 'str' holds a string with valid time
       format (is_valid_time(str) returns 1) */
    int h;
    int m;
    int s;
    sscanf(str, "%d:%2d:%2d", &h, &m, &s);
    return (h * 3600 + m * 60 + s);
}

static int inc_or_dec_timer(struct timer *t) {
    /* increment or decrement timer pointed to by 't' by a second */
    /* Returns 1 on success, 0 on failure */

    if (t->current != t->to) {
        if (t->up && t->to > t->current) {
            /* increment */
            t->current++;
            return 1;
        }

        else if (!t->up && t->current > t->to) {
            /* decrement */
            t->current--;
            return 1;
        }

        else {
            return 0;
        }
    }

    else { /* no more incrementing or decrementing is needed */
        return 0;
    }
}

static char time_str[9];
static char current_time[9];
static struct timer t;
static int hms[3];  // to be filled with current time.

#if defined(_WIN32)
#define MAX_PLAY_COUNT 10
static unsigned play_count = 0;
static DWORD WINAPI play_alarm_win(LPVOID data) {
    PlaySound(TEXT("alarm.wav"), NULL, SND_FILENAME);
    play_count++;
    if (strcmp(IupGetAttribute((Ihandle *)data, "ACTIVEWINDOW"), "NO") == 0 &&
        play_count < MAX_PLAY_COUNT) {
        CreateThread(NULL, 0, play_alarm_win, data, 0, NULL);
    } else if (play_count >= MAX_PLAY_COUNT)
        play_count = 0;

    return 0;
}

#elif defined(__linux__)
static void play_alarm_linux(void *ptr) {
    system("aplay alarm.wav");
}

#endif

static int timer_cb(Ihandle *ih) {
    Ihandle *from_time_label = IupGetAttributeHandle(ih, "FROM_TIME_LABEL");

    if (!inc_or_dec_timer(&t)) {
        IupSetAttribute(ih, "RUN", "NO");
        Ihandle *pr_button = IupGetDialogChild(ih, "PR_BUTTON");
        IupSetAttribute(pr_button, "ACTIVE", "NO");

#if defined(_WIN32)
        CreateThread(NULL, 0, play_alarm_win, IupGetDialog(ih), 0, NULL);

#elif defined(__linux__)
        pthread_t thrd;
        pthread_create(&thrd, NULL, play_alarm_linux, NULL);
#endif

    } else {
        sec_to_hms(t.current, hms);
        sprintf(current_time, "%02d:%02d:%02d", hms[0], hms[1], hms[2]);
        IupSetAttribute(from_time_label, "TITLE", current_time);
    }

    IupRefresh(ih);
    return IUP_DEFAULT;
}

static int on_timer_toggle(Ihandle *ih, int state) {
    if (state) {
        Ihandle *main_dlg = IupGetDialog(ih);
        Ihandle *input_hbox = IupGetDialogChild(main_dlg, "INPUT_HBOX");
        IupSetAttribute(input_hbox, "VISIBLE", "YES");
        IupSetAttribute(IupGetDialogChild(main_dlg, "TO_FROM_LABEL"), "TITLE",
                        "To");

        IupRefresh(ih);
    }

    return IUP_DEFAULT;
}

static int on_countdown_toggle(Ihandle *ih, int state) {
    if (state) {
        Ihandle *main_dlg = IupGetDialog(ih);
        Ihandle *input_hbox = IupGetDialogChild(main_dlg, "INPUT_HBOX");
        IupSetAttribute(input_hbox, "VISIBLE", "YES");
        IupSetAttribute(IupGetDialogChild(main_dlg, "TO_FROM_LABEL"), "TITLE",
                        "From");

        IupRefresh(ih);
    }

    return IUP_DEFAULT;
}

static int on_stopwatch_toggle(Ihandle *ih, int state) {
    if (state) {
        Ihandle *input_hbox = IupGetDialogChild(ih, "INPUT_HBOX");
        IupSetAttribute(input_hbox, "VISIBLE", "NO");
        IupSetAttribute(IupGetDialogChild(IupGetDialog(ih), "TO_FROM_LABEL"),
                        "TITLE", "");

        IupRefresh(ih);
    }

    return IUP_DEFAULT;
}

static int on_start_button_click(Ihandle *ih) {
    Ihandle *from_time_label = IupGetDialogChild(ih, "FROM_TIME_LABEL");
    Ihandle *to_time_label = IupGetDialogChild(ih, "TO_TIME_LABEL");

    int hours =
        atoi(IupGetAttribute(IupGetDialogChild(ih, "HOURS_SPIN"), "VALUE"));
    int minutes =
        atoi(IupGetAttribute(IupGetDialogChild(ih, "MINUTES_SPIN"), "VALUE"));
    int seconds =
        atoi(IupGetAttribute(IupGetDialogChild(ih, "SECONDS_SPIN"), "VALUE"));
    sprintf(time_str, "%02d:%02d:%02d", hours, minutes, seconds);

    Ihandle *timer = IupGetDialogChild(ih, "TIMER");
    Ihandle *timer_toggle = IupGetDialogChild(ih, "TIMER_TOGGLE");
    Ihandle *stopwatch_toggle = IupGetDialogChild(ih, "STOPWATCH_TOGGLE");
    Ihandle *countdown_toggle = IupGetDialogChild(ih, "COUNTDOWN_TOGGLE");

    if (strcmp(IupGetAttribute(timer_toggle, "VALUE"), "ON") == 0) {
        IupSetAttribute(from_time_label, "TITLE", "00:00:00");
        IupSetAttribute(to_time_label, "TITLE", time_str);
        t.current = 0;
        t.to = hms_to_sec(time_str);
        t.up = 1;
        IupSetAttribute(timer, "RUN", "YES");
    } else if (strcmp(IupGetAttribute(stopwatch_toggle, "VALUE"), "ON") == 0) {
        IupSetAttribute(from_time_label, "TITLE", "00:00:00");
        IupSetAttribute(to_time_label, "TITLE", "∞");
        t.current = 0;
        t.to = INT_MAX;
        t.up = 1;
        IupSetAttribute(timer, "RUN", "YES");
    } else if (strcmp(IupGetAttribute(countdown_toggle, "VALUE"), "ON") == 0) {
        IupSetAttribute(from_time_label, "TITLE", time_str);
        IupSetAttribute(to_time_label, "TITLE", "00:00:00");
        t.current = hms_to_sec(time_str);
        t.to = 0;
        t.up = 0;
        IupSetAttribute(timer, "RUN", "YES");
    }

    Ihandle *pr_button = IupGetDialogChild(ih, "PR_BUTTON");
    IupSetAttribute(pr_button, "ACTIVE", "YES");
    IupSetAttribute(pr_button, "TITLE", "Pause");

    IupRefresh(from_time_label);

    return IUP_DEFAULT;
}

static int on_pr_button_click(Ihandle *ih) {
    Ihandle *pr_button = IupGetDialogChild(ih, "PR_BUTTON");
    Ihandle *timer = IupGetDialogChild(ih, "TIMER");

    if (strcmp(IupGetAttribute(pr_button, "ACTIVE"), "YES") == 0) {
        if (strcmp(IupGetAttribute(timer, "RUN"), "YES") == 0) {
            IupSetAttribute(timer, "RUN", "NO");
            IupSetAttribute(pr_button, "TITLE", "Resume");
        } else if (strcmp(IupGetAttribute(timer, "RUN"), "NO") == 0) {
            IupSetAttribute(pr_button, "TITLE", "Pause");
            IupSetAttribute(timer, "RUN", "YES");
        }

        IupRefresh(ih);
    }

    return IUP_DEFAULT;
}

int main(void) {
    IupOpen(NULL, NULL);

    IupSetGlobal("UTF8MODE", "Yes");

    Ihandle *from_time_label = IupLabel("00:00:00");
    IupSetAttribute(from_time_label, "NAME", "FROM_TIME_LABEL");
    IupSetAttribute(from_time_label, "FONTSIZE", "20");
    Ihandle *separator = IupLabel(" - ");
    IupSetAttribute(separator, "FONTSIZE", "20");
    Ihandle *to_time_label = IupLabel("00:00:00");
    IupSetAttribute(to_time_label, "NAME", "TO_TIME_LABEL");
    IupSetAttribute(to_time_label, "FONTSIZE", "20");

    Ihandle *timer_status_hbox = IupHbox(IupFill(), from_time_label, separator,
                                         to_time_label, IupFill(), NULL);
    IupSetAttribute(timer_status_hbox, "CMARGIN", "10x10");

    Ihandle *timer_toggle = IupToggle("Timer", NULL);
    IupSetCallback(timer_toggle, "ACTION", (Icallback)on_timer_toggle);
    IupSetAttribute(timer_toggle, "NAME", "TIMER_TOGGLE");
    Ihandle *stopwatch_toggle = IupToggle("Stopwatch", NULL);
    IupSetCallback(stopwatch_toggle, "ACTION", (Icallback)on_stopwatch_toggle);
    IupSetAttribute(stopwatch_toggle, "NAME", "STOPWATCH_TOGGLE");
    Ihandle *countdown_toggle = IupToggle("Countdown", NULL);
    IupSetCallback(countdown_toggle, "ACTION", (Icallback)on_countdown_toggle);
    IupSetAttribute(countdown_toggle, "NAME", "COUNTDOWN_TOGGLE");

    Ihandle *toggles_hbox = IupHbox(IupFill(), timer_toggle, stopwatch_toggle,
                                    countdown_toggle, IupFill(), NULL);
    Ihandle *toggles = IupRadio(toggles_hbox);

    Ihandle *to_from_label = IupLabel("To");
    IupSetAttribute(to_from_label, "FONTSTYLE", "Bold");
    IupSetAttribute(to_from_label, "NAME", "TO_FROM_LABEL");

    Ihandle *hours_spin = IupText(NULL);
    IupSetAttribute(hours_spin, "NAME", "HOURS_SPIN");
    IupSetAttribute(hours_spin, "SPIN", "YES");
    // restrict the hours spin to value between 0-23
    IupSetAttribute(hours_spin, "SPINMAX", "23");
    // prevent invalid input typed manually
    IupSetAttribute(hours_spin, "MASKINT", "0:23");
    Ihandle *minutes_spin = IupText(NULL);
    IupSetAttribute(minutes_spin, "NAME", "MINUTES_SPIN");
    IupSetAttribute(minutes_spin, "SPIN", "YES");
    IupSetAttribute(minutes_spin, "SPINMAX", "59");
    IupSetAttribute(minutes_spin, "MASKINT", "0:59");
    Ihandle *seconds_spin = IupText(NULL);
    IupSetAttribute(seconds_spin, "NAME", "SECONDS_SPIN");
    IupSetAttribute(seconds_spin, "SPIN", "YES");
    IupSetAttribute(seconds_spin, "SPINMAX", "59");
    IupSetAttribute(seconds_spin, "MASKINT", "0:59");
    Ihandle *input_hbox = IupHbox(IupFill(), hours_spin, minutes_spin,
                                  seconds_spin, IupFill(), NULL);
    IupSetAttribute(input_hbox, "CMARGIN", "10x10");
    IupSetAttribute(input_hbox, "CGAP", "10");
    IupSetAttribute(input_hbox, "NAME", "INPUT_HBOX");

    Ihandle *start_button = IupButton("Start", NULL);
    IupSetAttribute(start_button, "SIZE", "50");
    IupSetCallback(start_button, "ACTION", (Icallback)on_start_button_click);
    Ihandle *pr_button = IupButton("Pause", NULL);
    IupSetAttribute(pr_button, "SIZE", "50");
    IupSetAttribute(pr_button, "ACTIVE", "NO");
    IupSetAttribute(pr_button, "NAME", "PR_BUTTON");
    IupSetCallback(pr_button, "ACTION", (Icallback)on_pr_button_click);
    Ihandle *buttons =
        IupHbox(IupFill(), start_button, pr_button, IupFill(), NULL);
    IupSetAttribute(buttons, "CGAP", "10");

    Ihandle *timer = IupTimer();
    IupSetAttribute(timer, "NAME", "TIMER");
    // we have to store from_time_label in timer because getting its handle
    // through IupGetDialogChild() returns a null pointer for some weird reason
    IupSetAttributeHandle(timer, "FROM_TIME_LABEL", from_time_label);
    IupSetAttribute(timer, "TIME", "1000");
    IupSetCallback(timer, "ACTION_CB", (Icallback)timer_cb);
    /* time will not be mapped unless its RUN attribute is set or it is appended
   to a container So this zbox is just to map the timer so we can get its
   handle later.
*/
    Ihandle *zbox = IupZbox(timer, NULL);

    Ihandle *main_vbox =
        IupVbox(IupFill(), timer_status_hbox, toggles, to_from_label,
                input_hbox, buttons, zbox, IupFill(), NULL);
    IupSetAttribute(main_vbox, "ALIGNMENT", "ACENTER");
    IupSetAttribute(main_vbox, "CMARGIN", "10x10");

    Ihandle *main_dlg = IupDialog(main_vbox);
    IupSetAttribute(main_dlg, "TITLE", "Timer");
    /* NOTE: The icon won't be displayed when viewing the executable through
     explorer.exe. A resource script must be created for that */
    IupSetAttribute(main_dlg, "ICON", "timer.ico");
    IupSetCallback(main_dlg, "K_CR", (Icallback)on_start_button_click);
    IupSetCallback(main_dlg, "K_p", (Icallback)on_pr_button_click);

    IupShowXY(main_dlg, IUP_CENTER, IUP_CENTER);
    IupMainLoop();

    IupDestroy(timer);
    IupClose();

    return 0;
}
