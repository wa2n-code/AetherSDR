#pragma once

#include <QWidget>

class QPushButton;
class QSlider;
class QLabel;

namespace AetherSDR {

class SliceModel;

// RX Applet — controls for a single receive slice.
//
// Layout (top to bottom):
//  • RX antenna selector (ANT1 / ANT2)
//  • Filter width presets (1.8 / 2.1 / 2.4 / 2.7 / 3.3 / 6.0 kHz)
//  • AGC mode (OFF / SLOW / MED / FAST)
//  • AF gain slider (audio output level)
//  • RF gain slider (IF gain)
//  • Squelch on/off + level slider
//  • DSP toggles: NB, NR, ANF
//  • RIT on/off + Hz offset with < > step buttons
//  • XIT on/off + Hz offset with < > step buttons
class RxApplet : public QWidget {
    Q_OBJECT

public:
    explicit RxApplet(QWidget* parent = nullptr);

    // Attach to a slice; pass nullptr to detach.
    void setSlice(SliceModel* slice);

signals:
    // Emitted when the user adjusts the AF gain slider (0–100).
    void afGainChanged(int value);

private:
    void buildUI();
    void connectSlice(SliceModel* s);
    void disconnectSlice(SliceModel* s);

    void applyFilterPreset(int widthHz);
    void updateFilterButtons();
    void updateAgcButtons();
    static QString formatHz(int hz);

    SliceModel* m_slice{nullptr};

    // ANT
    QPushButton* m_antBtns[2]{};   // [0]=ANT1  [1]=ANT2

    // Filter presets (Hz widths)
    static constexpr int FILTER_WIDTHS[6] = {1800, 2100, 2400, 2700, 3300, 6000};
    QPushButton* m_filterBtns[6]{};

    // AGC mode buttons (order: off / slow / med / fast)
    static constexpr const char* AGC_MODES[4] = {"off", "slow", "med", "fast"};
    QPushButton* m_agcBtns[4]{};

    // AF / RF gain
    QSlider*     m_afSlider{nullptr};
    QLabel*      m_afLabel{nullptr};
    QSlider*     m_rfSlider{nullptr};
    QLabel*      m_rfLabel{nullptr};

    // Squelch
    QPushButton* m_sqlBtn{nullptr};
    QSlider*     m_sqlSlider{nullptr};
    QLabel*      m_sqlLabel{nullptr};

    // DSP
    QPushButton* m_nbBtn{nullptr};
    QPushButton* m_nrBtn{nullptr};
    QPushButton* m_anfBtn{nullptr};

    // RIT
    QPushButton* m_ritOnBtn{nullptr};
    QPushButton* m_ritMinus{nullptr};
    QLabel*      m_ritLabel{nullptr};
    QPushButton* m_ritPlus{nullptr};

    // XIT
    QPushButton* m_xitOnBtn{nullptr};
    QPushButton* m_xitMinus{nullptr};
    QLabel*      m_xitLabel{nullptr};
    QPushButton* m_xitPlus{nullptr};

    static constexpr int RIT_STEP_HZ = 10;
};

} // namespace AetherSDR
