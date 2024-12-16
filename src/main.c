#include "core.h"
#include "core_position.h"
#include "core_render.h"

#include "parse_BSC5.h"
#include "term.h"
#include "stopwatch.h"
#include "../data/keplerian_elements.h"

#include <stdlib.h>
#include <getopt.h>
#include <signal.h>
#include <locale.h>
#include <stdbool.h>

// Options
static double longitude     = -71.057083;   // Boston, MA
static double latitude      = 42.361145;    // Boston, MA
static char *dt_string_utc  = NULL;         // UTC Datetime in yyyy-mm-ddThh:mm:ss format
static float threshold      = 3.0f;         // Stars brighter than this will be rendered
static float label_thresh   = 0.5f;         // Stars brighter than this will have labels
static int fps              = 24;           // Frames per second
static float animation_mult = 1.0f;         // Real time animation speed mult (e.g. 2 is 2x real time)

static double julian_date   = 0.0;          // Defaults to the current time if dt_string_utc unspecified

// Flags
static int unicode_flag     = TRUE;         // Only use ASCII characters
static int color_flag       = FALSE;        // Do not use color
static int grid_flag        = FALSE;        // Draw an azimuthal grid
static int constell_flag    = FALSE;        // Draw constellation figures

static volatile bool perform_resize = false;

static void catch_winch(int sig);
static void handle_resize(WINDOW *win);
static void parse_options(int argc, char *argv[]);
static void convert_options(void);
static void print_usage(void);

int main(int argc, char *argv[])
{
    // Parse command line args and convert to internal representations
    parse_options(argc, argv);
    convert_options();

    // Time for each frame in microseconds
    unsigned long dt = (unsigned long) (1.0 / fps * 1.0E6);

    // Render flags
    struct render_flags rf =
    {
        .unicode        = (unicode_flag != 0),
        .color          = (color_flag != 0),
        .label_thresh   = label_thresh,
    };

    // Initialize data structs
    unsigned int num_stars, num_const;

    struct entry        *BSC5_entries;
    struct star_name    *name_table;
    struct constell     *constell_table;
    struct star         *star_table;
    struct planet       *planet_table;
    struct moon         moon_object;
    int                 *num_by_mag;

    // Track success of functions
    bool s = true;

    s = s && parse_entries(&BSC5_entries, "../data/BSC5", &num_stars);
    s = s && generate_name_table(&name_table, "../data/BSC5_names.txt", num_stars);
    s = s && generate_constell_table(&constell_table, "../data/BSC5_constellations.txt", &num_const);
    s = s && generate_star_table(&star_table, BSC5_entries, name_table, num_stars);
    s = s && generate_planet_table(&planet_table, planet_elements, planet_rates, planet_extras);
    s = s && generate_moon_object(&moon_object, &moon_elements, &moon_rates);
    s = s && star_numbers_by_magnitude(&num_by_mag, star_table, num_stars);

    if (!s)
    {
        // At least one of the above functions failed, abort
        abort();
    }

    // This memory is no longer needed
    free(BSC5_entries);
    free_star_names(name_table, num_stars);

    // Terminal/System settings
    setlocale(LC_ALL, "");          // Required for unicode rendering
    signal(SIGWINCH, catch_winch);  // Capture window resizes

    // Ncurses initialization
    ncurses_init(color_flag != 0);
    WINDOW *win = newwin(0, 0, 0, 0);
    wtimeout(win, 0);   // Non-blocking read for wgetch
    win_resize_square(win, get_cell_aspect_ratio());
    win_position_center(win);

    // Render loop
    while (true)
    {
        struct sw_timestamp frame_begin;
        sw_gettime(&frame_begin);

        werase(win);

        if (perform_resize)
        {
            // Putting this after erasing the window reduces flickering
            handle_resize(win);
        }

        // Update object positions
        update_star_positions(star_table, num_stars, julian_date, latitude, longitude);
        update_planet_positions(planet_table, julian_date, latitude, longitude);
        update_moon_position(&moon_object, julian_date, latitude, longitude);
        update_moon_phase(planet_table, &moon_object, julian_date);

        // Render
        render_stars_stereo(win, &rf, star_table, num_stars, num_by_mag, threshold);
        if (constell_flag != 0)
        {
            render_constells(win, &rf, &constell_table, num_const, star_table);
        }
        render_planets_stereo(win, &rf, planet_table);
        render_moon_stereo(win, &rf, moon_object);
        if (grid_flag != 0)
        {
            render_azimuthal_grid(win, &rf);
        }
        else
        {
            render_cardinal_directions(win, &rf);
        }

        // Exit if ESC is pressed
        if ((wgetch(win)) == 27)
        {
            // Note: wgetch also calls wrefresh(win), so we want this at the
            // bottom after the virtual screen is updated
            break;
        }

        // TODO: this timing scheme *should* minimize any drift or divergence
        // between simulation time and realtime. Check this to make sure.

        // Increment "simulation" time
        const double microsec_per_day = 24.0 * 60.0 * 60.0 * 1.0E6;
        julian_date += (double) dt / microsec_per_day * animation_mult;

        // Determine time it took to update positions and render to screen
        struct sw_timestamp frame_end;
        sw_gettime(&frame_end);

        unsigned long long frame_time;
        sw_timediff_usec(frame_end, frame_begin, &frame_time);

        if (frame_time < dt)
        {
            sw_sleep(dt - frame_time);
        }
    }

    ncurses_kill();

    free_constells(constell_table, num_const);
    free_stars(star_table, num_stars);
    free_planets(planet_table, NUM_PLANETS);
    free_moon_object(moon_object);

    return 0;
}

void parse_options(int argc, char *argv[])
{
    // https://azrael.digipen.edu/~mmead/www/mg/getopt/index.html
    int c;
    while (true)
    {
        int option_index = 0;
        static struct option long_options[] =
        {
            {"latitude",        required_argument,  NULL,               'a'},
            {"datetime",        required_argument,  NULL,               'd'},
            {"fps",             required_argument,  NULL,               'f'},
            {"help",            no_argument,        NULL,               'h'},
            {"label-thresh",    required_argument,  NULL,               'l'},
            {"animation-mult",  required_argument,  NULL,               'm'},
            {"longitude",       required_argument,  NULL,               'o'},
            {"threshold",       required_argument,  NULL,               't'},
            {"color",           no_argument,        &color_flag,       TRUE},
            {"constellations",  no_argument,        &constell_flag,    TRUE},
            {"grid",            no_argument,        &grid_flag,        TRUE},
            {"no-unicode",      no_argument,        &unicode_flag,    FALSE},
            {NULL,              0,                  NULL,                 0}
        };

        c = getopt_long(argc, argv, "-:a:d:f:hl:m:o:t:", long_options, &option_index);
        if (c == -1)
            break;

        switch (c)
        {
        case 0:
            break;

        case 1:
            printf("Unrecognized regular argument '%s'\n", optarg);
            exit(EXIT_FAILURE);
            break;

        case 'a':
            latitude = atof(optarg);
            if (latitude < -90 || latitude > 90)
            {
                printf("Latitude out of range [-90°, 90°]\n");
                exit(EXIT_FAILURE);
            }
            break;

        case 'd':
            dt_string_utc = optarg;
            break;

        case 'f':
            fps = atoi(optarg);
            if (fps < 1)
            {
                printf("fps must be greater than or equal to 1\n");
                exit(EXIT_FAILURE);
            }
            break;

        case 'h':
            print_usage();
            exit(EXIT_SUCCESS);
            break;

        case 'l':
            label_thresh = (float) atof(optarg);
            break;

        case 'm':
            animation_mult = (float) atof(optarg);
            break;

        case 'o':
            longitude = atof(optarg);
            if (longitude < -180 || longitude > 180)
            {
                printf("Longitude out of range [-180°, 180°]\n");
                exit(EXIT_FAILURE);
            }
            break;

        case 't':
            threshold = (float) atof(optarg);
            break;

        case '?':
            if (optopt == 0)
            {
                printf("Unrecognized long option\n");
            }
            else
            {
                printf("Unrecognized option '%c'\n", optopt);
            }
            exit(EXIT_FAILURE);
            break;

        case ':':
            printf("Missing option for '%c'\n", optopt);
            exit(EXIT_FAILURE);
            break;

        default:
            printf("?? getopt returned character code 0%d ??\n", c);
            exit(EXIT_FAILURE);
            break;
        }
    }

    return ;
}

void convert_options(void)
{
    #ifndef M_PI
        #define M_PI 3.14159265358979323846
    #endif

    // Convert longitude and latitude to radians
    longitude *= M_PI / 180.0;
    latitude *= M_PI / 180.0;

    // Convert Gregorian calendar date to Julian date
    if (dt_string_utc == NULL)
    {
        // Set julian date to current time
        julian_date = current_julian_date();
    }
    else
    {
        struct tm datetime;
        bool parse_success = string_to_time(dt_string_utc, &datetime);
        if (!parse_success)
        {
            printf("Unable to parse datetime string '%s'\n", dt_string_utc);
            exit(EXIT_FAILURE);
        }
        julian_date = datetime_to_julian_date(&datetime);
    }

    return;
}

void catch_winch(int sig)
{
    perform_resize = true;
}

void handle_resize(WINDOW *win)
{
    // Resize ncurses internal terminal
    int y;
    int x;
    term_size(&y, &x);
    resizeterm(y, x);

    // ???
    wclear(win);
    wrefresh(win);

    // Check cell ratio
    float aspect = get_cell_aspect_ratio();

    // Resize/position application window
    win_resize_square(win, aspect);
    win_position_center(win);

    perform_resize = false;
}

void print_usage(void)
{
    printf("View stars, planets, and more, right in your terminal! ✨🪐\n");
    printf("\n");
    printf("Usage: starsaver [OPTION]...\n");
    printf("\n");
    printf("Exit: ESC\n");
    printf("\n");
    printf("Options:\n");
    printf(" -a, --latitude         [latitude]              Observer latitude in degrees. Positive North of the equator and negative South. Defaults to that of Boston, MA\n");
    printf(" -d, --datetime         [yyyy-mm-ddThh:mm:ss]   Observation time in UTC\n");
    printf(" -f, --fps              [fps]                   Frames per second. Defaults to 24\n");
    printf(" -h, --help                                     Print command line usage and exit\n");
    printf(" -l, --label-thresh     [threshold]             Stars with a brighter or equal magnitude to this threshold will be labeled (if a name is found). Defaults to 0.5\n");
    printf(" -m, --animation-mult   [multiplier]            Real time animation speed multiplier. Defaults to 1.0\n");
    printf(" -o, --longitude        [longitude]             Observer longitude in degrees. Positive East of the Prime Meridian and negative West.  Defaults to that of Boston, MA\n");
    printf(" -t, --threshold        [threshold]             Stars with a brighter or equal magnitude to this threshold will be drawn. Defaults to 3.0\n");
    printf("     --color                                    Draw planets with terminal colors\n");
    printf("     --constellations                           Draw constellations stick figures\n");
    printf("     --grid                                     Draw an azimuthal grid\n");
    printf("     --no-unicode                               Only render with ASCII characters\n");
    printf("\n");
    printf("Tips and tricks:\n");
    printf(" - Increasing performance: try using the no-unicode flag or increasing the fps to make movement appear smoother\n");
    printf(" - Decreasing CPU usage: try using the no-unicode flag, not rendering constellations, rendering fewer stars, and most of all, decreasing the fps\n");
}
