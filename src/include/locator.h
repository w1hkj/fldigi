#ifndef LOCATOR_H_
#define LOCATOR_H_

#include <config.h>

#if USE_HAMLIB
#  include <hamlib/rotator.h>
#else
#  ifdef __cplusplus
extern "C" {
#  endif

extern int qrb(double lon1, double lat1, double lon2, double lat2, double *distance, double *azimuth);

extern double distance_long_path(double distance);
extern double azimuth_long_path(double azimuth);

extern int longlat2locator(double longitude, double latitude, char *locator_res, int pair_count);
extern int locator2longlat(double *longitude, double *latitude, const char *locator);

extern double dms2dec(int degrees, int minutes, double seconds, int sw);
extern int dec2dms(double dec, int *degrees, int *minutes, double *seconds, int *sw);

extern int dec2dmmm(double dec, int *degrees, double *minutes, int *sw);
extern double dmmm2dec(int degrees, double minutes, int sw);
#  define HAMLIB_API /* empty */
enum rig_errcode_e { RIG_OK = 0, RIG_EINVAL };

#  ifdef __cplusplus
}
#  endif
#endif /* USE_HAMLIB */

#endif /* LOCATOR_H_ */
