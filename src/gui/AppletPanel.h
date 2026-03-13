#pragma once

#include <QWidget>

class QTabWidget;

namespace AetherSDR {

class SliceModel;
class RxApplet;

// AppletPanel — dockable right-side panel containing radio control applets.
//
// Currently implemented tabs: RX
// Planned: TX, P/CW, PHNE, EQ, ANLG
class AppletPanel : public QWidget {
    Q_OBJECT

public:
    explicit AppletPanel(QWidget* parent = nullptr);

    // Attach the active slice to all slice-dependent applets.
    void setSlice(SliceModel* slice);

    RxApplet* rxApplet() { return m_rxApplet; }

private:
    QTabWidget* m_tabs{nullptr};
    RxApplet*   m_rxApplet{nullptr};
};

} // namespace AetherSDR
