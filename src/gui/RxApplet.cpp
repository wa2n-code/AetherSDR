#include "RxApplet.h"
#include "models/SliceModel.h"

#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>

namespace AetherSDR {

// ─── Helpers ──────────────────────────────────────────────────────────────────

static QFrame* hLine()
{
    auto* f = new QFrame;
    f->setFrameShape(QFrame::HLine);
    f->setStyleSheet("QFrame { color: #1e2e3e; }");
    return f;
}

// Small checkable flat button used throughout the applet.
static QPushButton* mkToggle(const QString& text, QWidget* parent = nullptr)
{
    auto* b = new QPushButton(text, parent);
    b->setCheckable(true);
    b->setFlat(false);         // keep border so "checked" state is clearly visible
    b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    b->setFixedHeight(26);
    return b;
}

// Small non-checkable step button (< / >).
static QPushButton* mkStep(const QString& text, QWidget* parent = nullptr)
{
    auto* b = new QPushButton(text, parent);
    b->setFlat(false);
    b->setFixedSize(26, 26);
    return b;
}

// Shared stylesheet: blue when checked (used for ANT, filter, AGC).
static const QString kBlueActive =
    "QPushButton:checked { background-color: #0070c0; color: #ffffff; "
    "border: 1px solid #0090e0; }";

// Green when checked (used for NB/NR/ANF and SQL).
static const QString kGreenActive =
    "QPushButton:checked { background-color: #006040; color: #00ff88; "
    "border: 1px solid #00a060; }";

// Amber when checked (used for RIT/XIT on/off).
static const QString kAmberActive =
    "QPushButton:checked { background-color: #604000; color: #ffb800; "
    "border: 1px solid #906000; }";

// ─── Construction ─────────────────────────────────────────────────────────────

RxApplet::RxApplet(QWidget* parent) : QWidget(parent)
{
    buildUI();
}

void RxApplet::buildUI()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(6, 6, 6, 6);
    root->setSpacing(5);

    // ── Antenna ───────────────────────────────────────────────────────────────
    {
        auto* row = new QHBoxLayout;
        row->setSpacing(4);
        auto* lbl = new QLabel("ANT:");
        lbl->setFixedWidth(30);
        row->addWidget(lbl);

        const char* labels[] = {"ANT1", "ANT2"};
        for (int i = 0; i < 2; ++i) {
            m_antBtns[i] = mkToggle(labels[i]);
            m_antBtns[i]->setStyleSheet(kBlueActive);
            const QString ant = (i == 0) ? "ANT1" : "ANT2";
            connect(m_antBtns[i], &QPushButton::clicked, this, [this, ant](bool) {
                if (m_slice) m_slice->setRxAntenna(ant);
            });
            row->addWidget(m_antBtns[i]);
        }
        root->addLayout(row);
    }

    root->addWidget(hLine());

    // ── Filter presets ────────────────────────────────────────────────────────
    {
        auto* lbl = new QLabel("Filter:");
        lbl->setStyleSheet("color: #708090; font-size: 11px;");
        root->addWidget(lbl);

        auto* grid = new QGridLayout;
        grid->setSpacing(3);
        const char* labels[] = {"1.8K","2.1K","2.4K","2.7K","3.3K","6.0K"};
        for (int i = 0; i < 6; ++i) {
            m_filterBtns[i] = mkToggle(labels[i]);
            m_filterBtns[i]->setStyleSheet(kBlueActive);
            const int w = FILTER_WIDTHS[i];
            connect(m_filterBtns[i], &QPushButton::clicked, this, [this, w](bool) {
                applyFilterPreset(w);
            });
            grid->addWidget(m_filterBtns[i], i / 3, i % 3);
        }
        root->addLayout(grid);
    }

    root->addWidget(hLine());

    // ── AGC mode ──────────────────────────────────────────────────────────────
    {
        auto* row = new QHBoxLayout;
        row->setSpacing(3);
        auto* lbl = new QLabel("AGC:");
        lbl->setFixedWidth(34);
        lbl->setStyleSheet("color: #708090; font-size: 11px;");
        row->addWidget(lbl);

        const char* labels[] = {"Off","Slw","Med","Fst"};
        for (int i = 0; i < 4; ++i) {
            m_agcBtns[i] = mkToggle(labels[i]);
            m_agcBtns[i]->setStyleSheet(kBlueActive);
            const QString mode = AGC_MODES[i];
            connect(m_agcBtns[i], &QPushButton::clicked, this, [this, mode](bool) {
                if (m_slice) m_slice->setAgcMode(mode);
            });
            row->addWidget(m_agcBtns[i]);
        }
        root->addLayout(row);
    }

    root->addWidget(hLine());

    // ── AF / RF gain ──────────────────────────────────────────────────────────
    auto mkGainRow = [&](const QString& labelText, QSlider*& slider, QLabel*& valLbl,
                         int initVal) {
        auto* row = new QHBoxLayout;
        row->setSpacing(4);
        auto* lbl = new QLabel(labelText);
        lbl->setFixedWidth(18);
        lbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        row->addWidget(lbl);

        slider = new QSlider(Qt::Horizontal);
        slider->setRange(0, 100);
        slider->setValue(initVal);
        row->addWidget(slider, 1);

        valLbl = new QLabel(QString::number(initVal));
        valLbl->setFixedWidth(28);
        valLbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        row->addWidget(valLbl);
        root->addLayout(row);
    };

    mkGainRow("AF", m_afSlider, m_afLabel, 70);
    mkGainRow("RF", m_rfSlider, m_rfLabel, 0);

    connect(m_afSlider, &QSlider::valueChanged, this, [this](int v) {
        m_afLabel->setText(QString::number(v));
        emit afGainChanged(v);
    });
    connect(m_rfSlider, &QSlider::valueChanged, this, [this](int v) {
        m_rfLabel->setText(QString::number(v));
        if (m_slice) m_slice->setRfGain(static_cast<float>(v));
    });

    root->addWidget(hLine());

    // ── Squelch ───────────────────────────────────────────────────────────────
    {
        auto* row = new QHBoxLayout;
        row->setSpacing(4);

        m_sqlBtn = mkToggle("SQL");
        m_sqlBtn->setFixedWidth(44);
        m_sqlBtn->setStyleSheet(kGreenActive);
        row->addWidget(m_sqlBtn);

        m_sqlSlider = new QSlider(Qt::Horizontal);
        m_sqlSlider->setRange(0, 100);
        m_sqlSlider->setValue(20);
        row->addWidget(m_sqlSlider, 1);

        m_sqlLabel = new QLabel("20");
        m_sqlLabel->setFixedWidth(28);
        m_sqlLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        row->addWidget(m_sqlLabel);

        root->addLayout(row);

        connect(m_sqlBtn, &QPushButton::toggled, this, [this](bool on) {
            if (m_slice) m_slice->setSquelch(on, m_sqlSlider->value());
        });
        connect(m_sqlSlider, &QSlider::valueChanged, this, [this](int v) {
            m_sqlLabel->setText(QString::number(v));
            if (m_slice && m_sqlBtn->isChecked())
                m_slice->setSquelch(true, v);
        });
    }

    root->addWidget(hLine());

    // ── DSP toggles ───────────────────────────────────────────────────────────
    {
        auto* row = new QHBoxLayout;
        row->setSpacing(4);

        m_nbBtn  = mkToggle("NB");
        m_nrBtn  = mkToggle("NR");
        m_anfBtn = mkToggle("ANF");

        for (auto* b : {m_nbBtn, m_nrBtn, m_anfBtn})
            b->setStyleSheet(kGreenActive);

        connect(m_nbBtn,  &QPushButton::toggled, this, [this](bool on) {
            if (m_slice) m_slice->setNb(on);
        });
        connect(m_nrBtn,  &QPushButton::toggled, this, [this](bool on) {
            if (m_slice) m_slice->setNr(on);
        });
        connect(m_anfBtn, &QPushButton::toggled, this, [this](bool on) {
            if (m_slice) m_slice->setAnf(on);
        });

        row->addWidget(m_nbBtn);
        row->addWidget(m_nrBtn);
        row->addWidget(m_anfBtn);
        root->addLayout(row);
    }

    root->addWidget(hLine());

    // ── RIT ───────────────────────────────────────────────────────────────────
    {
        auto* row = new QHBoxLayout;
        row->setSpacing(4);

        m_ritOnBtn = mkToggle("RIT");
        m_ritOnBtn->setFixedWidth(38);
        m_ritOnBtn->setStyleSheet(kAmberActive);
        row->addWidget(m_ritOnBtn);

        m_ritMinus = mkStep("<");
        row->addWidget(m_ritMinus);

        m_ritLabel = new QLabel("0 Hz");
        m_ritLabel->setAlignment(Qt::AlignCenter);
        m_ritLabel->setMinimumWidth(60);
        m_ritLabel->setStyleSheet(
            "QLabel { background: #0a0a18; border: 1px solid #1e2e3e; "
            "border-radius: 3px; padding: 2px 4px; }");
        row->addWidget(m_ritLabel, 1);

        m_ritPlus = mkStep(">");
        row->addWidget(m_ritPlus);

        root->addLayout(row);

        connect(m_ritOnBtn, &QPushButton::toggled, this, [this](bool on) {
            if (m_slice) m_slice->setRit(on, m_slice->ritFreq());
        });
        connect(m_ritMinus, &QPushButton::clicked, this, [this] {
            if (!m_slice) return;
            m_slice->setRit(m_ritOnBtn->isChecked(), m_slice->ritFreq() - RIT_STEP_HZ);
        });
        connect(m_ritPlus, &QPushButton::clicked, this, [this] {
            if (!m_slice) return;
            m_slice->setRit(m_ritOnBtn->isChecked(), m_slice->ritFreq() + RIT_STEP_HZ);
        });
    }

    // ── XIT ───────────────────────────────────────────────────────────────────
    {
        auto* row = new QHBoxLayout;
        row->setSpacing(4);

        m_xitOnBtn = mkToggle("XIT");
        m_xitOnBtn->setFixedWidth(38);
        m_xitOnBtn->setStyleSheet(kAmberActive);
        row->addWidget(m_xitOnBtn);

        m_xitMinus = mkStep("<");
        row->addWidget(m_xitMinus);

        m_xitLabel = new QLabel("0 Hz");
        m_xitLabel->setAlignment(Qt::AlignCenter);
        m_xitLabel->setMinimumWidth(60);
        m_xitLabel->setStyleSheet(
            "QLabel { background: #0a0a18; border: 1px solid #1e2e3e; "
            "border-radius: 3px; padding: 2px 4px; }");
        row->addWidget(m_xitLabel, 1);

        m_xitPlus = mkStep(">");
        row->addWidget(m_xitPlus);

        root->addLayout(row);

        connect(m_xitOnBtn, &QPushButton::toggled, this, [this](bool on) {
            if (m_slice) m_slice->setXit(on, m_slice->xitFreq());
        });
        connect(m_xitMinus, &QPushButton::clicked, this, [this] {
            if (!m_slice) return;
            m_slice->setXit(m_xitOnBtn->isChecked(), m_slice->xitFreq() - RIT_STEP_HZ);
        });
        connect(m_xitPlus, &QPushButton::clicked, this, [this] {
            if (!m_slice) return;
            m_slice->setXit(m_xitOnBtn->isChecked(), m_slice->xitFreq() + RIT_STEP_HZ);
        });
    }

    root->addStretch();
}

// ─── Slice wiring ─────────────────────────────────────────────────────────────

void RxApplet::setSlice(SliceModel* slice)
{
    if (m_slice) disconnectSlice(m_slice);
    m_slice = slice;
    if (m_slice) connectSlice(m_slice);
}

void RxApplet::connectSlice(SliceModel* s)
{
    // Antenna
    {
        const bool ant1 = (s->rxAntenna() != "ANT2");
        QSignalBlocker b0(m_antBtns[0]), b1(m_antBtns[1]);
        m_antBtns[0]->setChecked(ant1);
        m_antBtns[1]->setChecked(!ant1);
    }
    connect(s, &SliceModel::rxAntennaChanged, this, [this](const QString& ant) {
        QSignalBlocker b0(m_antBtns[0]), b1(m_antBtns[1]);
        m_antBtns[0]->setChecked(ant != "ANT2");
        m_antBtns[1]->setChecked(ant == "ANT2");
    });

    // Filter
    updateFilterButtons();
    connect(s, &SliceModel::filterChanged, this, [this](int, int) {
        updateFilterButtons();
    });

    // AGC
    updateAgcButtons();
    connect(s, &SliceModel::agcModeChanged, this, [this](const QString&) {
        updateAgcButtons();
    });

    // RF gain
    {
        QSignalBlocker b(m_rfSlider);
        m_rfSlider->setValue(static_cast<int>(s->rfGain()));
        m_rfLabel->setText(QString::number(static_cast<int>(s->rfGain())));
    }

    // Squelch
    {
        QSignalBlocker b1(m_sqlBtn), b2(m_sqlSlider);
        m_sqlBtn->setChecked(s->squelchOn());
        m_sqlSlider->setValue(s->squelchLevel());
        m_sqlLabel->setText(QString::number(s->squelchLevel()));
    }
    connect(s, &SliceModel::squelchChanged, this, [this](bool on, int level) {
        QSignalBlocker b1(m_sqlBtn), b2(m_sqlSlider);
        m_sqlBtn->setChecked(on);
        m_sqlSlider->setValue(level);
        m_sqlLabel->setText(QString::number(level));
    });

    // DSP toggles
    {
        QSignalBlocker b1(m_nbBtn), b2(m_nrBtn), b3(m_anfBtn);
        m_nbBtn->setChecked(s->nbOn());
        m_nrBtn->setChecked(s->nrOn());
        m_anfBtn->setChecked(s->anfOn());
    }
    connect(s, &SliceModel::nbChanged,  this, [this](bool on) {
        QSignalBlocker b(m_nbBtn);  m_nbBtn->setChecked(on);
    });
    connect(s, &SliceModel::nrChanged,  this, [this](bool on) {
        QSignalBlocker b(m_nrBtn);  m_nrBtn->setChecked(on);
    });
    connect(s, &SliceModel::anfChanged, this, [this](bool on) {
        QSignalBlocker b(m_anfBtn); m_anfBtn->setChecked(on);
    });

    // RIT
    {
        QSignalBlocker b(m_ritOnBtn);
        m_ritOnBtn->setChecked(s->ritOn());
        m_ritLabel->setText(formatHz(s->ritFreq()));
    }
    connect(s, &SliceModel::ritChanged, this, [this](bool on, int hz) {
        QSignalBlocker b(m_ritOnBtn);
        m_ritOnBtn->setChecked(on);
        m_ritLabel->setText(formatHz(hz));
    });

    // XIT
    {
        QSignalBlocker b(m_xitOnBtn);
        m_xitOnBtn->setChecked(s->xitOn());
        m_xitLabel->setText(formatHz(s->xitFreq()));
    }
    connect(s, &SliceModel::xitChanged, this, [this](bool on, int hz) {
        QSignalBlocker b(m_xitOnBtn);
        m_xitOnBtn->setChecked(on);
        m_xitLabel->setText(formatHz(hz));
    });
}

void RxApplet::disconnectSlice(SliceModel* s)
{
    s->disconnect(this);
}

// ─── Private helpers ──────────────────────────────────────────────────────────

QString RxApplet::formatHz(int hz)
{
    return (hz >= 0 ? "+" : "") + QString::number(hz) + " Hz";
}

void RxApplet::applyFilterPreset(int widthHz)
{
    if (!m_slice) return;

    int lo, hi;
    const QString& mode = m_slice->mode();

    if (mode == "LSB" || mode == "DIGL" || mode == "CWL") {
        lo = -widthHz;
        hi = 0;
    } else if (mode == "CW") {
        // Centred 200 Hz above carrier (600 Hz sidetone pitch)
        lo = 200;
        hi = 200 + widthHz;
    } else {
        // USB, DIGU, AM, FM, DIG, etc.
        lo = 0;
        hi = widthHz;
    }

    m_slice->setFilterWidth(lo, hi);
}

void RxApplet::updateFilterButtons()
{
    const int width = m_slice ? (m_slice->filterHigh() - m_slice->filterLow()) : -1;
    for (int i = 0; i < 6; ++i) {
        QSignalBlocker sb(m_filterBtns[i]);
        m_filterBtns[i]->setChecked(width >= 0 &&
                                    std::abs(width - FILTER_WIDTHS[i]) <= 150);
    }
}

void RxApplet::updateAgcButtons()
{
    const QString cur = m_slice ? m_slice->agcMode() : "";
    for (int i = 0; i < 4; ++i) {
        QSignalBlocker sb(m_agcBtns[i]);
        m_agcBtns[i]->setChecked(cur == AGC_MODES[i]);
    }
}

} // namespace AetherSDR
