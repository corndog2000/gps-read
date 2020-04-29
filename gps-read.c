// gps_read.c
// Joe Schroedl
// February 25, 2020
// https://gist.github.com/corndog2000/5caa522f9a8fc75bb9d61e518d4d1cb8

#include <stdio.h>
#include <sys/types.h>
#include <termios.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

// NMEA GPS data parsing libraries
// You can get it here: https://github.com/corndog2000/libnmea
#include <nmea.h>
#include <nmea/gpgll.h>
#include <nmea/gpgga.h>
#include <nmea/gprmc.h>
#include <nmea/gpgsv.h>
#include <nmea/gpvtg.h>
#include <nmea/gptxt.h>
#include <nmea/gpgsa.h>

// Serial port of the GPS module
#define SERIAL_DEVICE "/dev/ttyACM0"

// Clear the terminal
#define clear() printf("\033[H\033[J")

int main() {
    struct termios serial_port_settings;
    int fd;
    int retval;
    char buf[256];

    fd = open(SERIAL_DEVICE, O_RDWR);
    if (fd < 0) {
        perror("Failed to open SERIAL_DEVICE");
        exit(1);
    } else {
        printf("Connected to device at: ");
        printf(SERIAL_DEVICE);
        printf("\n");
    }

    retval = tcgetattr(fd, &serial_port_settings);
    if (retval < 0) {
        perror("Failed to get termios structure");
        exit(2);
    }

    //setting baud rate to B115200
    retval = cfsetospeed(&serial_port_settings, B115200);
    if (retval < 0) {
        perror("Failed to set 115200 output baud rate");
        exit(3);
    }
    retval = cfsetispeed(&serial_port_settings, B115200);
    if (retval < 0) {
        perror("Failed to set 115200 input baud rate");
        exit(4);
    }

    serial_port_settings.c_lflag |= ICANON;
    //Disable ECHO
    serial_port_settings.c_lflag &= ~(ECHO | ECHOE);
    retval = tcsetattr(fd, TCSANOW, &serial_port_settings);

    if (retval < 0) {
        perror("Failed to set serial attributes");
        exit(5);
    }
    printf("Successfully set the baud rate\n");

    clear();

    // Keep reading serial data
    while (1) {
        int nline_pos = 0;

        // Set all spaces in buf to 0
        memset(buf, 0, sizeof (buf));

        retval = read(fd, buf, sizeof (buf));

        /*  Format Received String  */

        // If the first character is not equal to a "$" then make it a "$"
        if (buf[0] != '$') {
            //memmove(buf, buf+1, strlen(buf));
            buf[0] = '$';
        }

        // Find the first occurrence of a newline character
        char * newline = strchr(buf, '\n');
        if (newline) {
            newline++; // advance ptr to next character after \n
            *newline = '\0';
        } else {
            continue;
        }

        // Pointer to structure containing the parsed data
        nmea_s *data;

        // Parse it...
        data = nmea_parse(buf, strlen(buf), 0);

        if (NULL == data) {
            printf("Failed to parse sentence!\n");
            continue;
        }

        if (NMEA_GPTXT == data->type) {
            nmea_gptxt_s *gptxt = (nmea_gptxt_s *) data;

            printf("GPTXT Sentence:\n");
            printf("---------------\n");

            printf("\tID: %d %d %d\n", gptxt->id_00, gptxt->id_01, gptxt->id_02);
            printf("\t%s\n", gptxt->text);
        }

        if (NMEA_GPRMC == data->type) {
            clear();
            
            nmea_gprmc_s *gprmc = (nmea_gprmc_s *) data;

            printf("GPRMC sentence\n");
            printf("---------------\n");

            printf("Longitude:\n");
            printf("  Degrees: %d\n", gprmc->longitude.degrees);
            printf("  Minutes: %f\n", gprmc->longitude.minutes);
            printf("  Cardinal: %c\n", (char) gprmc->longitude.cardinal);
            printf("Latitude:\n");
            printf("  Degrees: %d\n", gprmc->latitude.degrees);
            printf("  Minutes: %f\n", gprmc->latitude.minutes);
            printf("  Cardinal: %c\n\n", (char) gprmc->latitude.cardinal);
            strftime(buf, sizeof (buf), "%d %b %T %Y", &gprmc->date_time);
            printf("Date & Time: %s\n\n", buf);
            printf("Speed, in Knots: %f:\n", gprmc->gndspd_knots);
            printf("Track, in degrees: %f\n\n", gprmc->track_deg);
        }

        if (NMEA_GPVTG == data->type) {
            nmea_gpvtg_s *gpvtg = (nmea_gpvtg_s *) data;

            printf("GPVTG Sentence:\n");
            printf("---------------\n");

            printf("  Track [deg]:   %.2lf\n", gpvtg->track_deg);
            printf("  Speed [kmph]:  %.2lf\n", gpvtg->gndspd_kmph);
            printf("  Speed [knots]: %.2lf\n\n", gpvtg->gndspd_knots);
        }

        if (NMEA_GPGSA == data->type) {
            nmea_gpgsa_s *gpgsa = (nmea_gpgsa_s *) data;

            printf("GPGSA Sentence:\n");
            printf("---------------\n");

            printf("  Mode: %c\n", gpgsa->mode);
            printf("  Fix Key: 1 = no fix, 2 = 2D, 3 = 3D\n");
            printf("  Fix:  %d\n", gpgsa->fixtype);
            printf("  PDOP: %.2lf\n", gpgsa->pdop);
            printf("  HDOP: %.2lf\n", gpgsa->hdop);
            printf("  VDOP: %.2lf\n", gpgsa->vdop);

            // 
            if (gpgsa->fixtype != 1) {
                clear();
            }

        }

        if (NMEA_GPGSV == data->type) {
            nmea_gpgsv_s *gpgsv = (nmea_gpgsv_s *) data;

            printf("GPGSV Sentence:\n");
            printf("\tTotal sentences: %u\n", gpgsv->sentences);
            printf("\tCurrent sentence:  %u\n", gpgsv->sentence_number);
            printf("\tSatellites:  %u\n", gpgsv->satellites);
            //printf("Prn | Elevation Deg. | Azimuth Deg. | Signal Strength dB\n");
            printf("\t#1:  %u %d %d %d\n", gpgsv->s0_prn, gpgsv->s0_el_deg, gpgsv->s0_az_deg, gpgsv->s0_snr_db);
            printf("\t#2:  %u %d %d %d\n", gpgsv->s1_prn, gpgsv->s1_el_deg, gpgsv->s1_az_deg, gpgsv->s1_snr_db);
            printf("\t#3:  %u %d %d %d\n", gpgsv->s2_prn, gpgsv->s2_el_deg, gpgsv->s2_az_deg, gpgsv->s2_snr_db);
            printf("\t#4:  %u %d %d %d\n", gpgsv->s3_prn, gpgsv->s3_el_deg, gpgsv->s3_az_deg, gpgsv->s3_snr_db);


        }

        /*

        
        if (NMEA_GPGLL == data->type) {
            printf("GPGLL sentence\n");

            nmea_gpgll_s *gpgll = (nmea_gpgll_s *) data;

            printf("Longitude:\n");
            printf("  Degrees: %d\n", gpgll->longitude.degrees);
            printf("  Minutes: %f\n", gpgll->longitude.minutes);
            printf("  Cardinal: %c\n", (char) gpgll->longitude.cardinal);
            printf("Latitude:\n");
            printf("  Degrees: %d\n", gpgll->latitude.degrees);
            printf("  Minutes: %f\n", gpgll->latitude.minutes);
            printf("  Cardinal: %c\n", (char) gpgll->latitude.cardinal);
            strftime(buf, sizeof (buf), "%H:%M:%S", &gpgll->time);
            printf("Time: %s\n", buf);

        }
         */


        nmea_free(data);
    }

    close(fd);
    return 0;
}
