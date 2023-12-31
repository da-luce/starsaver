#include "core_render.h"

#include "core.h"
#include "coord.h"
#include "drawing.h"

#include <stdlib.h>
#include <math.h>
#include <ncurses.h>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

void render_object_stereo(WINDOW *win, struct object_base *object, struct render_flags *rf)
{
    double theta_sphere, phi_sphere;
    horizontal_to_spherical(object->azimuth, object->altitude,
                            &theta_sphere, &phi_sphere);

    double radius_polar, theta_polar;
    project_stereographic_north(1.0, theta_sphere, phi_sphere,
                                &radius_polar, &theta_polar);

    int y, x;
    int height, width;
    getmaxyx(win, height, width);
    polar_to_win(radius_polar, theta_polar,
                 height, width,
                 &y, &x);

    // Cache object coordinates
    object->y = y;
    object->x = x;

    // If outside projection, ignore
    if (fabs(radius_polar) > 1)
    {
        return;
    }

    bool use_color = rf->color && object->color_pair != 0;

    if (use_color)
    {
        wattron(win, COLOR_PAIR(object->color_pair));
    }

    // Draw object
    if (rf->unicode)
    {
        mvwaddstr(win, y, x, object->symbol_unicode);
    }
    else
    {
        mvwaddch(win, y, x, object->symbol_ASCII);
    }

    // Draw label
    // FIXME: labels wrap around side, cause flickering
    if (object->label != NULL)
    {
        mvwaddstr(win, y - 1, x + 1, object->label);
    }

    if (use_color)
    {
        wattroff(win, COLOR_PAIR(object->color_pair));
    }

    return;
}

void render_stars_stereo(WINDOW *win, struct render_flags *rf,
                         struct star *star_table, int num_stars,
                         int *num_by_mag, float threshold)
{
    int i;
    for (i = 0; i < num_stars; ++i)
    {
        int catalog_num = num_by_mag[i];
        int table_index = catalog_num - 1;

        struct star *star = &star_table[table_index];

        if (star->magnitude > threshold)
        {
            continue;
        }

        // FIXME: this is hacky
        if (star->magnitude > rf->label_thresh)
        {
            star->base.label = NULL;
        }

        render_object_stereo(win, &star->base, rf);
    }

    return;
}

void render_constells(WINDOW *win, struct render_flags *rf,
                      struct constell **constell_table, int num_const,
                      struct star *star_table)
{
    int i;
    for (i = 0; i < num_const; ++i)
    {
        struct constell constellation = (*constell_table)[i];
        unsigned int num_segments = constellation.num_segments;

        unsigned int j;
        for (j = 0; j < num_segments * 2; j += 2)
        {

            int catalog_num_a = constellation.star_numbers[j];
            int catalog_num_b = constellation.star_numbers[j + 1];

            int table_index_a = catalog_num_a - 1;
            int table_index_b = catalog_num_b - 1;

            struct star star_a = star_table[table_index_a];
            struct star star_b = star_table[table_index_b];

            int ya = (int)star_a.base.y;
            int xa = (int)star_a.base.x;
            int yb = (int)star_b.base.y;
            int xb = (int)star_b.base.x;

            // This implies the star is not being drawn due to it's magnitude
            // FIXME: this is hacky...
            if ((ya == 0 && xa == 0) || (yb == 0 && xb == 0))
            {
                continue;
            }

            // Draw line if reasonable length (avoid printing crazy long lines)
            // TODO: is there a cleaner way to do this (perhaps if checking if
            // one of the stars is in the window?)
            // FIXME: this is too nested
            double line_length = sqrt(pow(ya - yb, 2) + pow(xa - xb, 2));
            if (line_length < 10000)
            {
                if (rf->unicode)
                {
                    draw_line_smooth(win, ya, xa, yb, xb);
                    mvwaddstr(win, ya, xa, "○");
                    mvwaddstr(win, yb, xb, "○");
                }
                else
                {
                    draw_line_ASCII(win, ya, xa, yb, xb);
                    mvwaddch(win, ya, xa, '+');
                    mvwaddch(win, yb, xb, '+');
                }
            }
        }
    }
}

void render_planets_stereo(WINDOW *win, struct render_flags *rf, struct planet *planet_table)
{
    // Render planets so that closest are drawn on top
    int i;
    for (i = NUM_PLANETS - 1; i >= 0; --i)
    {
        // Skip rendering the Earth--we're on the Earth! The geocentric
        // coordinates of the Earth are (0.0, 0.0, 0.0) and plotting the "Earth"
        // simply traces along the ecliptic at the approximate hour angle
        if (i == EARTH)
        {
            continue;
        }

        struct planet planet_data = planet_table[i];
        render_object_stereo(win, &planet_data.base, rf);
    }

    return;
}

void render_moon_stereo(WINDOW *win, struct render_flags *rf, struct moon moon_object)
{
    render_object_stereo(win, &moon_object.base, rf);

    return;
}

int gcd(int a, int b)
{
    int temp;
    while (b != 0)
    {
        temp = a % b;

        a = b;
        b = temp;
    }
    return a;
}

int compare_angles(const void *a, const void *b)
{
    int x = *(int *)a;
    int y = *(int *)b;
    return (90 / gcd(x, 90)) < (90 / gcd(y, 90));
}

void render_azimuthal_grid(WINDOW *win, struct render_flags *rf)
{
    const double to_rad = M_PI / 180.0;

    int height, width;
    getmaxyx(win, height, width);
    int maxy = height - 1;
    int maxx = width - 1;

    int rad_vertical = round(maxy / 2.0);
    int rad_horizontal = round(maxx / 2.0);

    // Possible step sizes in degrees (multiples of 5 and factors of 90)
    int step_sizes[5] = {10, 15, 30, 45, 90};
    int length = sizeof(step_sizes) / sizeof(step_sizes[0]);

    // Minumum number of rows separating grid line (at end of window)
    int min_height = 10;

    // Set the step size to the smallest desirable increment
    int inc;
    int i;
    for (i = length - 1; i >= 0; --i)
    {
        inc = step_sizes[i];
        if (round(rad_vertical * sin(inc * to_rad)) < min_height)
        {
            inc = step_sizes[--i]; // Go back to previous increment
            break;
        }
    }

    // Sort grid angles in the first quadrant by rendering priority
    int number_angles = 90 / inc + 1;
    int *angles = malloc(number_angles * sizeof(int));

    // int i;
    for (i = 0; i < number_angles; ++i)
    {
        angles[i] = inc * i;
    }
    qsort(angles, number_angles, sizeof(int), compare_angles);

    // Draw angles in all four quadrants
    int quad;
    for (quad = 0; quad < 4; ++quad)
    {
        int i;
        for (i = 0; i < number_angles; ++i)
        {
            int angle = angles[i] + 90 * quad;

            int y = rad_vertical - round(rad_vertical * sin(angle * to_rad));
            int x = rad_horizontal + round(rad_horizontal * cos(angle * to_rad));

            if (rf->unicode)
            {
                draw_line_smooth(win, y, x, rad_vertical, rad_horizontal);
            }
            else
            {
                draw_line_ASCII(win, y, x, rad_vertical, rad_horizontal);
            }

            int str_len = snprintf(NULL, 0, "%d", angle);
            char *label = malloc(str_len + 1);

            snprintf(label, str_len + 1, "%d", angle);

            // Offset to avoid truncating string
            int y_off = (y < rad_vertical) ? 1 : -1;
            int x_off = (x < rad_horizontal) ? 0 : -(str_len - 1);

            mvwaddstr(win, y, x + x_off, label);

            free(label);
        }
    }

    // while (angle <= 90.0)
    // {
    //     int rad_x = rad_horizontal * angle / 90.0;
    //     int rad_x = rad_vertical * angle / 90.0;
    //     // draw_ellipse(win, win->_maxy/2, win->_maxx/2, 20, 20, unicode_flag);
    //     angle += inc;
    // }
}

void render_cardinal_directions(WINDOW *win, struct render_flags *rf)
{
    // Render horizon directions

    if (rf->color)
    {
        wattron(win, COLOR_PAIR(5));
    }

    int height, width;
    getmaxyx(win, height, width);
    int maxy = height - 1;
    int maxx = width - 1;

    int half_maxy = round(maxy / 2.0);
    int half_maxx = round(maxx / 2.0);

    mvwaddch(win, 0, half_maxx, 'N');
    mvwaddch(win, half_maxy, width - 1, 'W');
    mvwaddch(win, height - 1, half_maxx, 'S');
    mvwaddch(win, half_maxy, 0, 'E');

    if (rf->color)
    {
        wattroff(win, COLOR_PAIR(5));
    }
}
