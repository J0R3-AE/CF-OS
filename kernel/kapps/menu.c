#include "drivers/tty.h"
#include "drivers/kbd.h"
#include "libk/printf.h"
#include "libk/string.h"
#include "libk/types.h"
#include <stddef.h>

/*
 * MiniOS KApp Menu (Demo Edition)
 *
 * Purpose:
 * - safe temporary UI demo
 * - no filesystem scans
 * - no shell launch
 * - no blocking backend calls
 * - exercises menu navigation, rendering, and kernel-facing placeholders
 *
 * Keys:
 *   KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT
 *   KEY_ENTER, KEY_ESC, KEY_BACKSPACE, Q
 */

typedef enum
{
    MENU_MAIN = 0,
    MENU_DIAGNOSTICS,
    MENU_DISPLAY,
    MENU_INPUT,
    MENU_STORAGE,
    MENU_PROCESS,
    MENU_POWER,
    MENU_ABOUT,

    MENU_CPU_INFO,
    MENU_MEM_INFO,
    MENU_TTY_TEST,
    MENU_TIMER_TEST,

    MENU_KEYBOARD_TEST,
    MENU_KEYMAP_TEST,

    MENU_VFS_OVERVIEW,
    MENU_MOUNT_OVERVIEW,
    MENU_FS_OVERVIEW,

    MENU_PROC_OVERVIEW,
    MENU_THREAD_OVERVIEW,

    MENU_REBOOT_CONFIRM,
    MENU_SHUTDOWN_CONFIRM,
    MENU_EXIT
} menu_state_t;

typedef void (*menu_action_t)(void);

typedef struct
{
    const char *label;
    const char *desc;
    menu_state_t next;
    menu_action_t action;
} menu_item_t;

typedef struct
{
    const char *title;
    const char *subtitle;
    const menu_item_t *items;
    int count;
} menu_page_t;

static menu_state_t state = MENU_MAIN;
static int selected = 0;
static int running = 1;
static int dirty = 1;
static u32 demo_ticks = 0;

/* -----------------------------
   Helpers
   ----------------------------- */

static void set_color(uint8_t fg, uint8_t bg)
{
    TTY_setcolor(fg, bg);
}

static void clear_screen(void)
{
    set_color(TTY_COLOR_WHITE, TTY_COLOR_BLACK);
    TTY_clear();
}

static int key_is_up(int key)
{
    return key == KEY_UP || key == 'w' || key == 'W';
}

static int key_is_down(int key)
{
    return key == KEY_DOWN || key == 's' || key == 'S';
}

static int key_is_enter(int key)
{
    return key == KEY_ENTER || key == '\n' || key == '\r';
}

static int key_is_esc(int key)
{
    return key == KEY_ESC || key == KEY_BACKSPACE || key == 27;
}

static void clamp_selection(int count)
{
    if (count <= 0)
    {
        selected = 0;
        return;
    }

    if (selected < 0)
        selected = 0;
    if (selected >= count)
        selected = count - 1;
}

static void goto_state(menu_state_t next)
{
    state = next;
    selected = 0;
    dirty = 1;
}

static void back_to_main(void)
{
    goto_state(MENU_MAIN);
}

static void draw_header(const char *title, const char *subtitle)
{
    TTY_setpos(0, 2);
    set_color(TTY_COLOR_WHITE, TTY_COLOR_BLACK);
    printf("MiniOS Kernel Demo Menu");

    TTY_setpos(1, 2);
    printf("%s", title ? title : "");

    TTY_setpos(2, 2);
    printf("%s", subtitle ? subtitle : "");

    TTY_setpos(3, 2);
    printf("------------------------------------------------------------");
}

static void draw_desc_bar(const char *desc)
{
    TTY_setpos(4, 2);
    set_color(TTY_COLOR_WHITE, TTY_COLOR_LIGHT_BLUE);
    printf("%s", desc ? desc : "");
}

static void draw_footer(const char *status)
{
    size_t rows = 0, cols = 0;
    TTY_getsize(&rows, &cols);
    (void)cols;

    if (rows < 4)
        rows = 25;

    set_color(TTY_COLOR_LIGHT_GREY, TTY_COLOR_BLACK);
    TTY_setpos(rows - 3, 2);
    printf("Arrows / W-S   Enter=Select   Esc=Back   Q=Quit");

    TTY_setpos(rows - 2, 2);
    printf("Status: %s", status ? status : "");
}

static void draw_page(const menu_page_t *page, int sel, const char *status)
{
    clear_screen();
    draw_header(page->title, page->subtitle);

    const char *desc = "";
    if (page->items && page->count > 0 && sel >= 0 && sel < page->count)
        desc = page->items[sel].desc ? page->items[sel].desc : "";

    draw_desc_bar(desc);

    for (int i = 0; i < page->count; i++)
    {
        TTY_setpos(6 + i, 4);

        if (i == sel)
        {
            set_color(TTY_COLOR_WHITE, TTY_COLOR_LIGHT_BLUE);
            printf("> %s", page->items[i].label ? page->items[i].label : "");
        }
        else
        {
            set_color(TTY_COLOR_WHITE, TTY_COLOR_BLACK);
            printf("  %s", page->items[i].label ? page->items[i].label : "");
        }
    }

    draw_footer(status);
}

static void draw_info_page(const char *title,
                           const char *subtitle,
                           const char *desc,
                           const char *line1,
                           const char *line2,
                           const char *status)
{
    clear_screen();
    draw_header(title, subtitle);
    draw_desc_bar(desc);
    set_color(TTY_COLOR_WHITE, TTY_COLOR_BLACK);

    TTY_setpos(7, 4);
    printf("%s", line1 ? line1 : "");

    if (line2)
    {
        TTY_setpos(9, 4);
        printf("%s", line2);
    }

    draw_footer(status);
}

static void action_show_cpu(void)
{
    draw_info_page(
        "CPU Info",
        "Diagnostics",
        "Static CPU demo panel.",
        "CPU: x86 / i386 demo target",
        "APIC/PIT/IRQ status can be added later.",
        "CPU diagnostics");
}

static void action_show_mem(void)
{
    draw_info_page(
        "Memory Info",
        "Diagnostics",
        "Static memory demo panel.",
        "PMM / VMM / heap overview goes here.",
        "Add live counters when ready.",
        "Memory diagnostics");
}

static void action_show_tty(void)
{
    clear_screen();
    draw_header("TTY Test", "Display");
    draw_desc_bar("Visual test for console output.");
    set_color(TTY_COLOR_WHITE, TTY_COLOR_BLACK);

    TTY_setpos(7, 4);
    printf("This page tests text placement.");
    TTY_setpos(8, 4);
    printf("If you can read this, TTY output works.");
    TTY_setpos(10, 4);
    printf("Demo tick: %u", demo_ticks);

    draw_footer("TTY test");
}

static void action_show_timer(void)
{
    clear_screen();
    draw_header("Timer Test", "Diagnostics");
    draw_desc_bar("Kernel timer / scheduler demo panel.");
    set_color(TTY_COLOR_WHITE, TTY_COLOR_BLACK);

    TTY_setpos(7, 4);
    printf("Timer tick counter demo.");
    TTY_setpos(8, 4);
    printf("demo_ticks = %u", demo_ticks);
    TTY_setpos(10, 4);
    printf("Add PIT-driven updates later.");

    draw_footer("Timer diagnostics");
}

static void action_show_keyboard(void)
{
    draw_info_page(
        "Keyboard Test",
        "Input",
        "Keyboard input demo panel.",
        "Press arrows, Enter, Esc, or Q.",
        "The menu state updates live.",
        "Keyboard diagnostics");
}

static void action_show_keymap(void)
{
    draw_info_page(
        "Keymap Test",
        "Input",
        "Keyboard mapping demo panel.",
        "W/S also act like Up/Down.",
        "Enter selects, Esc goes back.",
        "Keymap diagnostics");
}

static void action_show_vfs(void)
{
    draw_info_page(
        "VFS Overview",
        "Storage",
        "Virtual filesystem demo panel.",
        "Root path resolution, lookup, and open semantics belong here.",
        "Keep this page as a placeholder until the live browser lands.",
        "VFS diagnostics");
}

static void action_show_mounts(void)
{
    draw_info_page(
        "Mount Overview",
        "Storage",
        "Mount table demo panel.",
        "This page is intentionally static for now.",
        "It is safe and will not scan live kernel lists.",
        "Mount diagnostics");
}

static void action_show_fs(void)
{
    draw_info_page(
        "Filesystem Overview",
        "Storage",
        "Filesystem driver demo panel.",
        "RAMFS / EXT2 / FAT can be shown here later.",
        "For now, this is a stable placeholder.",
        "Filesystem diagnostics");
}

static void action_show_proc(void)
{
    draw_info_page(
        "Process Overview",
        "Process",
        "Process / thread demo panel.",
        "Scheduler state, ready queue, and sleepers can be shown here.",
        "No live queries yet; safe static display only.",
        "Process diagnostics");
}

static void action_show_threads(void)
{
    draw_info_page(
        "Thread Overview",
        "Process",
        "Thread demo panel.",
        "A future live table can list TIDs, states, and CPU time.",
        "This page is static for now.",
        "Thread diagnostics");
}

static void action_reboot_now(void)
{
    clear_screen();
    set_color(TTY_COLOR_WHITE, TTY_COLOR_BLACK);
    TTY_setpos(6, 4);
    printf("Reboot requested.");
    TTY_setpos(8, 4);
    printf("Hook your reboot routine here.");
    running = 0;
    state = MENU_EXIT;
}

static void action_shutdown_now(void)
{
    clear_screen();
    set_color(TTY_COLOR_WHITE, TTY_COLOR_BLACK);
    TTY_setpos(6, 4);
    printf("Shutdown requested.");
    TTY_setpos(8, 4);
    printf("Hook your shutdown routine here.");
    running = 0;
    state = MENU_EXIT;
}

/* -----------------------------
   Pages
   ----------------------------- */

static const menu_item_t main_items[] = {
    {"Diagnostics", "CPU, memory, timer, keyboard", MENU_DIAGNOSTICS, NULL},
    {"Display", "TTY / text rendering", MENU_DISPLAY, NULL},
    {"Input", "Keyboard test pages", MENU_INPUT, NULL},
    {"Storage", "VFS, mounts, filesystems", MENU_STORAGE, NULL},
    {"Process", "Threads and scheduler overview", MENU_PROCESS, NULL},
    {"Power", "Reboot / shutdown", MENU_POWER, NULL},
    {"About", "Build and kernel info", MENU_ABOUT, NULL},
    {"Exit", "Leave the menu", MENU_EXIT, NULL},
};

static const menu_item_t diag_items[] = {
    {"CPU Info", "CPU / interrupt overview", MENU_CPU_INFO, action_show_cpu},
    {"Memory Info", "Heap and paging overview", MENU_MEM_INFO, action_show_mem},
    {"Timer Test", "PIT / tick demo", MENU_TIMER_TEST, action_show_timer},
    {"Keyboard Test", "Input and navigation demo", MENU_KEYBOARD_TEST, action_show_keyboard},
    {"Back", "Return to main menu", MENU_MAIN, NULL},
};

static const menu_item_t display_items[] = {
    {"TTY Test", "Text rendering sanity check", MENU_TTY_TEST, action_show_tty},
    {"Back", "Return to main menu", MENU_MAIN, NULL},
};

static const menu_item_t input_items[] = {
    {"Keyboard Test", "Live key navigation hints", MENU_KEYBOARD_TEST, action_show_keyboard},
    {"Keymap Test", "W/S and arrows mapping", MENU_KEYMAP_TEST, action_show_keymap},
    {"Back", "Return to main menu", MENU_MAIN, NULL},
};

static const menu_item_t storage_items[] = {
    {"VFS Overview", "Path lookup and file API", MENU_VFS_OVERVIEW, action_show_vfs},
    {"Mount Overview", "Mount table placeholder", MENU_MOUNT_OVERVIEW, action_show_mounts},
    {"Filesystem Overview", "RAMFS / EXT2 / FAT", MENU_FS_OVERVIEW, action_show_fs},
    {"Back", "Return to main menu", MENU_MAIN, NULL},
};

static const menu_item_t process_items[] = {
    {"Process Overview", "Scheduler and task summary", MENU_PROC_OVERVIEW, action_show_proc},
    {"Thread Overview", "Thread state summary", MENU_THREAD_OVERVIEW, action_show_threads},
    {"Back", "Return to main menu", MENU_MAIN, NULL},
};

static const menu_item_t power_items[] = {
    {"Reboot", "Restart the machine", MENU_REBOOT_CONFIRM, NULL},
    {"Shutdown", "Power off the machine", MENU_SHUTDOWN_CONFIRM, NULL},
    {"Back", "Return to main menu", MENU_MAIN, NULL},
};

static const menu_item_t about_items[] = {
    {"About", "Kernel demo information", MENU_ABOUT, NULL},
    {"Back", "Return to main menu", MENU_MAIN, NULL},
};

static const menu_item_t confirm_yesno_items[] = {
    {"Yes", "Confirm the action", MENU_EXIT, NULL},
    {"No", "Cancel and go back", MENU_MAIN, NULL},
};

static const menu_page_t main_page = {
    .title = "Main Menu",
    .subtitle = "Select a category",
    .items = main_items,
    .count = 8,
};

static const menu_page_t diag_page = {
    .title = "Diagnostics",
    .subtitle = "Kernel health checks",
    .items = diag_items,
    .count = 5,
};

static const menu_page_t display_page = {
    .title = "Display",
    .subtitle = "TTY and rendering tests",
    .items = display_items,
    .count = 2,
};

static const menu_page_t input_page = {
    .title = "Input",
    .subtitle = "Keyboard tests",
    .items = input_items,
    .count = 3,
};

static const menu_page_t storage_page = {
    .title = "Storage",
    .subtitle = "VFS / mounts / filesystems",
    .items = storage_items,
    .count = 4,
};

static const menu_page_t process_page = {
    .title = "Process",
    .subtitle = "Scheduler / threads",
    .items = process_items,
    .count = 3,
};

static const menu_page_t power_page = {
    .title = "Power",
    .subtitle = "Power options",
    .items = power_items,
    .count = 3,
};

static const menu_page_t confirm_page = {
    .title = "Confirm",
    .subtitle = "Confirmation required",
    .items = confirm_yesno_items,
    .count = 2,
};

/* -----------------------------
   Input handlers
   ----------------------------- */

static void handle_page_nav(int key, int count)
{
    if (key_is_up(key))
    {
        selected--;
        clamp_selection(count);
        dirty = 1;
        return;
    }

    if (key_is_down(key))
    {
        selected++;
        clamp_selection(count);
        dirty = 1;
        return;
    }
}

static void handle_main(int key)
{
    handle_page_nav(key, main_page.count);

    if (key_is_esc(key) || key == 'q' || key == 'Q')
    {
        state = MENU_EXIT;
        running = 0;
        return;
    }

    if (key_is_enter(key))
    {
        switch (selected)
        {
        case 0:
            goto_state(MENU_DIAGNOSTICS);
            break;
        case 1:
            goto_state(MENU_DISPLAY);
            break;
        case 2:
            goto_state(MENU_INPUT);
            break;
        case 3:
            goto_state(MENU_STORAGE);
            break;
        case 4:
            goto_state(MENU_PROCESS);
            break;
        case 5:
            goto_state(MENU_POWER);
            break;
        case 6:
            goto_state(MENU_ABOUT);
            break;
        case 7:
            state = MENU_EXIT;
            running = 0;
            break;
        }
    }
}

static void handle_diag(int key)
{
    handle_page_nav(key, diag_page.count);

    if (key_is_esc(key))
    {
        back_to_main();
        return;
    }

    if (key_is_enter(key))
    {
        if (diag_page.items[selected].action)
            diag_page.items[selected].action();

        if (selected == 4)
            back_to_main();
    }
}

static void handle_display(int key)
{
    handle_page_nav(key, display_page.count);

    if (key_is_esc(key))
    {
        back_to_main();
        return;
    }

    if (key_is_enter(key))
    {
        if (display_page.items[selected].action)
            display_page.items[selected].action();

        if (selected == 1)
            back_to_main();
    }
}

static void handle_input(int key)
{
    handle_page_nav(key, input_page.count);

    if (key_is_esc(key))
    {
        back_to_main();
        return;
    }

    if (key_is_enter(key))
    {
        if (input_page.items[selected].action)
            input_page.items[selected].action();

        if (selected == 2)
            back_to_main();
    }
}

static void handle_storage(int key)
{
    handle_page_nav(key, storage_page.count);

    if (key_is_esc(key))
    {
        back_to_main();
        return;
    }

    if (key_is_enter(key))
    {
        if (storage_page.items[selected].action)
            storage_page.items[selected].action();

        if (selected == 3)
            back_to_main();
    }
}

static void handle_process(int key)
{
    handle_page_nav(key, process_page.count);

    if (key_is_esc(key))
    {
        back_to_main();
        return;
    }

    if (key_is_enter(key))
    {
        if (process_page.items[selected].action)
            process_page.items[selected].action();

        if (selected == 2)
            back_to_main();
    }
}

static void handle_power(int key)
{
    handle_page_nav(key, power_page.count);

    if (key_is_esc(key))
    {
        back_to_main();
        return;
    }

    if (key_is_enter(key))
    {
        switch (selected)
        {
        case 0:
            goto_state(MENU_REBOOT_CONFIRM);
            break;
        case 1:
            goto_state(MENU_SHUTDOWN_CONFIRM);
            break;
        case 2:
            back_to_main();
            break;
        }
    }
}

static void handle_about(int key)
{
    if (key_is_esc(key) || key_is_enter(key))
        back_to_main();
}

static void handle_confirm_page(int key, int rebooting)
{
    if (key_is_esc(key))
    {
        back_to_main();
        return;
    }

    if (key == 'y' || key == 'Y')
    {
        if (rebooting)
            action_reboot_now();
        else
            action_shutdown_now();
        return;
    }

    if (key == 'n' || key == 'N')
    {
        back_to_main();
        return;
    }

    handle_page_nav(key, confirm_page.count);

    if (key_is_enter(key))
    {
        if (selected == 0)
        {
            if (rebooting)
                action_reboot_now();
            else
                action_shutdown_now();
        }
        else
        {
            back_to_main();
        }
    }
}

/* -----------------------------
   Render dispatcher
   ----------------------------- */

static void render_current(void)
{
    switch (state)
    {
    case MENU_MAIN:
        draw_page(&main_page, selected, "Demo home");
        break;
    case MENU_DIAGNOSTICS:
        draw_page(&diag_page, selected, "CPU / memory / timer / keyboard");
        break;
    case MENU_DISPLAY:
        draw_page(&display_page, selected, "TTY and rendering tests");
        break;
    case MENU_INPUT:
        draw_page(&input_page, selected, "Keyboard navigation tests");
        break;
    case MENU_STORAGE:
        draw_page(&storage_page, selected, "VFS / mounts / filesystems");
        break;
    case MENU_PROCESS:
        draw_page(&process_page, selected, "Scheduler / threads");
        break;
    case MENU_POWER:
        draw_page(&power_page, selected, "Power controls");
        break;
    case MENU_ABOUT:
        draw_info_page(
            "About",
            "Kernel demo",
            "Build and feature summary.",
            "This menu is temporary and safe.",
            "It exercises UI, input, and basic control flow.",
            "About page");
        break;
    case MENU_CPU_INFO:
        action_show_cpu();
        break;
    case MENU_MEM_INFO:
        action_show_mem();
        break;
    case MENU_TTY_TEST:
        action_show_tty();
        break;
    case MENU_TIMER_TEST:
        action_show_timer();
        break;
    case MENU_KEYBOARD_TEST:
        action_show_keyboard();
        break;
    case MENU_KEYMAP_TEST:
        action_show_keymap();
        break;
    case MENU_VFS_OVERVIEW:
        action_show_vfs();
        break;
    case MENU_MOUNT_OVERVIEW:
        action_show_mounts();
        break;
    case MENU_FS_OVERVIEW:
        action_show_fs();
        break;
    case MENU_PROC_OVERVIEW:
        action_show_proc();
        break;
    case MENU_THREAD_OVERVIEW:
        action_show_threads();
        break;
    case MENU_REBOOT_CONFIRM:
        draw_info_page(
            "Reboot",
            "Power",
            "Confirm reboot.",
            "Press Y to reboot.",
            "Press N or Esc to cancel.",
            "Reboot confirmation");
        break;
    case MENU_SHUTDOWN_CONFIRM:
        draw_info_page(
            "Shutdown",
            "Power",
            "Confirm shutdown.",
            "Press Y to shutdown.",
            "Press N or Esc to cancel.",
            "Shutdown confirmation");
        break;
    case MENU_EXIT:
        clear_screen();
        draw_header("Exit", "Leaving the menu");
        draw_desc_bar("Exiting the demo menu.");
        set_color(TTY_COLOR_WHITE, TTY_COLOR_BLACK);
        TTY_setpos(7, 4);
        printf("Leaving the menu...");
        draw_footer("Exiting");
        break;
    }
}

/* -----------------------------
   Main app
   ----------------------------- */

void kapps_menu_main(void *arg)
{
    (void)arg;

    printf("Menu started\n");

    while (running)
    {
        int key = kbd_try_getchar();

        if (key < 0)
        {
            if (dirty)
            {
                render_current();
                dirty = 0;
            }

            demo_ticks++;
            asm volatile("hlt");
            continue;
        }

        switch (state)
        {
        case MENU_MAIN:
            handle_main(key);
            break;
        case MENU_DIAGNOSTICS:
            handle_diag(key);
            break;
        case MENU_DISPLAY:
            handle_display(key);
            break;
        case MENU_INPUT:
            handle_input(key);
            break;
        case MENU_STORAGE:
            handle_storage(key);
            break;
        case MENU_PROCESS:
            handle_process(key);
            break;
        case MENU_POWER:
            handle_power(key);
            break;
        case MENU_ABOUT:
            handle_about(key);
            break;
        case MENU_REBOOT_CONFIRM:
            handle_confirm_page(key, 1);
            break;
        case MENU_SHUTDOWN_CONFIRM:
            handle_confirm_page(key, 0);
            break;
        default:
            if (key_is_esc(key))
                back_to_main();
            break;
        }

        dirty = 1;
    }

    clear_screen();
    set_color(TTY_COLOR_WHITE, TTY_COLOR_BLACK);
    TTY_setpos(10, 4);
    printf("Menu exiting");
}