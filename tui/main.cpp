#include <clocale>

#include "tui.h"

int main() {
  std::setlocale(LC_ALL, "");
  TuiApp app;
  return app.run();
}
