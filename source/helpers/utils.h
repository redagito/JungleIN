#ifndef UTILS_H
#define UTILS_H

#include <string>

/*!
 * Utils
 * Fonctions diverses et vari�es (mais utiles)
 * @static
 */
class Utils {
public:
  static char *getFileContent(std::string path);

  static bool USE_ANISO;
  static unsigned int MAX_ANISO;
  static int QUALITY;

  static double TARGET_FPS;

  static double calculFPS();

  static double random(double min, double max);
};

#endif // UTILS_H
