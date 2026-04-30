#pragma once

#include <QDialog>
#include <QMainWindow>

namespace dsfw {

struct FramelessHelper {
    static void apply(QMainWindow *window);

    static void applyToDialog(QDialog *dialog);
};

} // namespace dsfw
