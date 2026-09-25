#include <QApplication>

#include "window.h"

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  app.setOrganizationName("Mizux");
  app.setApplicationName("QHana");

  MainWindow main;
  main.show();
  return app.exec();
}
