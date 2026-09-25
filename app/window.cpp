#include "window.h"

#include <QApplication>
#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QFormLayout>
#include <QGraphicsLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QShortcut>
#include <QTabWidget>
#include <QTextEdit>
#include <QTimer>
#include <QToolTip>
#include <QTreeWidget>
#include <QWidget>
#include <algorithm>
#include <array>
#include <format>
#include <string>

#include "config.h"
#include "qhana.h"
#include "window-cb.h"

// ============================================================================
// ScoreChartView Implementation
// ============================================================================

ScoreChartView::ScoreChartView(bool compact, QWidget* parent)
    : QChartView(parent), _compact(compact) {
  _setupChart();
}

void ScoreChartView::_setupChart() {
  _chart = new QChart();
  _chart->legend()->hide();
  _chart->setBackgroundBrush(QBrush(Qt::black));
  _chart->setPlotAreaBackgroundBrush(QBrush(Qt::black));
  _chart->setPlotAreaBackgroundVisible(true);
  _chart->setBackgroundRoundness(0);

  if (_compact) {
    _chart->setMargins(QMargins(2, 2, 2, 2));
    if (_chart->layout()) {
      _chart->layout()->setContentsMargins(0, 0, 0, 0);
    }
    QFont titleFont = _chart->titleFont();
    titleFont.setPointSize(8);
    titleFont.setBold(true);
    _chart->setTitleFont(titleFont);
    _chart->setTitleBrush(QBrush(QColor(0, 255, 0)));
  } else {
    _chart->setMargins(QMargins(8, 8, 8, 8));
    QFont titleFont = _chart->titleFont();
    titleFont.setPointSize(11);
    titleFont.setBold(true);
    _chart->setTitleFont(titleFont);
    _chart->setTitleBrush(QBrush(QColor(0, 255, 0)));
  }

  _axis_x = new QValueAxis();
  _axis_x->setRange(1, 10);
  _axis_x->setTickCount(6);
  _axis_x->setLabelFormat("%d");
  _axis_x->setLabelsColor(QColor(180, 180, 180));
  _axis_x->setGridLineColor(QColor(40, 40, 40));
  _axis_x->setLinePenColor(QColor(100, 100, 100));
  if (_compact) {
    _axis_x->setLabelsVisible(false);
    _axis_x->setGridLineVisible(false);
    _axis_x->setLineVisible(false);
  } else {
    _axis_x->setTitleText("Round");
    _axis_x->setTitleBrush(QBrush(QColor(180, 180, 180)));
  }

  _axis_y = new QValueAxis();
  _axis_y->setLabelFormat("%d");
  _axis_y->setLabelsColor(QColor(180, 180, 180));
  _axis_y->setGridLineColor(QColor(40, 40, 40));
  _axis_y->setLinePenColor(QColor(100, 100, 100));
  if (_compact) {
    _axis_y->setLabelsVisible(false);
    _axis_y->setGridLineVisible(false);
    _axis_y->setLineVisible(false);
  }

  _chart->addAxis(_axis_x, Qt::AlignBottom);
  _chart->addAxis(_axis_y, Qt::AlignLeft);

  _series_zero = new QLineSeries();
  _series_zero->setPen(QPen(QColor(128, 128, 128), 1, Qt::DotLine));

  _series_data = new QLineSeries();
  _series_data->setPen(
      QPen(QColor(255, 204, 0), _compact ? 1.5 : 2.0, Qt::SolidLine));

  _series_points = new QScatterSeries();
  _series_points->setMarkerShape(QScatterSeries::MarkerShapeCircle);
  _series_points->setMarkerSize(_compact ? 3.5 : 5.0);
  _series_points->setColor(QColor(255, 204, 0));
  _series_points->setBorderColor(QColor(255, 204, 0));

  _chart->addSeries(_series_zero);
  _chart->addSeries(_series_data);
  _chart->addSeries(_series_points);

  for (auto* s : std::initializer_list<QAbstractSeries*>{
           _series_zero, _series_data, _series_points}) {
    s->attachAxis(_axis_x);
    s->attachAxis(_axis_y);
  }

  setChart(_chart);
  setRenderHint(QPainter::Antialiasing);
  setFrameShape(QFrame::NoFrame);
  setContentsMargins(0, 0, 0, 0);
}

QString ScoreChartView::itemName(int item_idx) {
  switch (item_idx) {
    case ITEM_TOTAL_SCORE:
      return "Cumulative Score";
    case ITEM_PLAYER_ROUND:
      return "Your Round Points";
    case ITEM_BOT_ROUND:
      return "Computer Round Points";
    default:
      return "Cumulative Score";
  }
}

void ScoreChartView::setItemIndex(int idx) {
  int clamped = std::clamp(idx, 0, TOTAL_ITEMS - 1);
  if (_item_idx != clamped) {
    _item_idx = clamped;
    emit itemChanged(_item_idx);
  }
}

void ScoreChartView::updateChart(const GameState& gameState) {
  _series_zero->clear();
  _series_data->clear();
  _series_points->clear();

  _chart->setTitle(itemName(_item_idx));

  const std::vector<int>* vec = &gameState.score_history;
  int current_val = gameState.score;
  if (_item_idx == ITEM_PLAYER_ROUND) {
    vec = &gameState.player_round_scores;
    current_val = gameState.players[0].result.score;
  } else if (_item_idx == ITEM_BOT_ROUND) {
    vec = &gameState.bot_round_scores;
    current_val = gameState.players[1].result.score;
  }

  int count = static_cast<int>(vec->size());
  int max_x = std::max(10, count + 1);
  _axis_x->setRange(1, max_x);

  _series_zero->append(1, 0);
  _series_zero->append(max_x, 0);

  double min_val = 0.0;
  double max_val = 10.0;
  auto record = [&](double v) {
    if (v < min_val) min_val = v;
    if (v > max_val) max_val = v;
  };

  if (count == 0) {
    _series_data->append(1, current_val);
    _series_points->append(1, current_val);
    record(current_val);
  } else {
    for (int i = 0; i < count; ++i) {
      double v = (*vec)[i];
      _series_data->append(i + 1, v);
      _series_points->append(i + 1, v);
      record(v);
    }
    if (gameState.phase != GamePhase::RoundOver) {
      _series_data->append(count + 1, current_val);
      _series_points->append(count + 1, current_val);
      record(current_val);
    }
  }

  double span = std::max(10.0, max_val - min_val);
  double pad = std::max(2.0, span * 0.15);
  _axis_y->setRange(min_val - pad, max_val + pad);
}

void ScoreChartView::mousePressEvent(QMouseEvent* event) {
  if (_compact && event->button() == Qt::LeftButton) {
    setItemIndex((_item_idx + 1) % TOTAL_ITEMS);
    event->accept();
    return;
  }
  QChartView::mousePressEvent(event);
}

void ScoreChartView::contextMenuEvent(QContextMenuEvent* event) {
  QMenu menu(this);
  QAction* zoomAction = menu.addAction("Zoom in...");
  connect(zoomAction, &QAction::triggered, this,
          [this]() { emit zoomRequested(_item_idx); });
  menu.addSeparator();

  for (int i = 0; i < TOTAL_ITEMS; ++i) {
    QAction* act = menu.addAction(itemName(i));
    act->setCheckable(true);
    act->setChecked(i == _item_idx);
    connect(act, &QAction::triggered, this, [this, i]() { setItemIndex(i); });
  }
  menu.exec(event->globalPos());
}

// ============================================================================
// HanafudaBoardWidget Implementation
// ============================================================================

HanafudaBoardWidget::HanafudaBoardWidget(GameState& gameState, QWidget* parent)
    : QWidget(parent), _gameState(gameState) {
  setFixedSize(640, 488);
  setMouseTracking(true);
  _loadAssets();
}

void HanafudaBoardWidget::_loadAssets() {
  if (!_cards_img.load(":/gfx/cards.bmp")) {
    _cards_img.load("data/gfx/cards.bmp");
  }
  if (!_back_img.load(":/gfx/back.bmp")) {
    _back_img.load("data/gfx/back.bmp");
  }
}

void HanafudaBoardWidget::setSelectedHandIndex(int idx) {
  int hand_sz = static_cast<int>(_gameState.players[0].hand.size());
  if (hand_sz <= 0) {
    _selected_hand_idx = 0;
  } else {
    _selected_hand_idx = std::clamp(idx, 0, hand_sz - 1);
  }
  update();
}

QPixmap HanafudaBoardWidget::renderCardPixmap(const Card& c, int w,
                                              int h) const {
  QPixmap pix(w, h);
  pix.fill(Qt::transparent);
  QPainter p(&pix);
  p.setRenderHint(QPainter::SmoothPixmapTransform, true);

  int cw = w - 1;
  int ch = h - 1;

  // Drop shadow matching SDLHana RenderCard
  p.fillRect(cw, 1, 1, ch, QColor(10, 10, 10));
  p.fillRect(1, ch, cw, 1, QColor(10, 10, 10));

  if (!_cards_img.isNull()) {
    int pw = _cards_img.width() / 4;
    int ph = _cards_img.height() / 13;
    QRect src;
    if (!c.is_valid()) {
      src = QRect(pw * 2, ph * 12, pw, ph);
    } else {
      src = QRect((c.id() & 3) * pw, (c.month() - 1) * ph, pw, ph);
    }
    p.drawImage(QRect(0, 0, cw, ch), _cards_img, src);
  } else {
    // Fallback vector rendering if BMP missing
    p.fillRect(0, 0, cw, ch, c.is_valid() ? QColor(245, 238, 220) : QColor(140, 65, 25));
    p.setPen(QColor(30, 30, 30));
    p.drawRect(0, 0, cw - 1, ch - 1);
    if (c.is_valid()) {
      p.drawText(QRect(2, 2, cw - 4, ch - 4), Qt::AlignCenter | Qt::TextWordWrap,
                 QString("%1M\n%2").arg(c.month()).arg(
                     QString::fromStdString(card_short_label(c))));
    }
  }

  if (c.render_effect & static_cast<unsigned int>(CardEffect::Dark)) {
    p.fillRect(0, 0, cw, ch, QColor(0, 0, 0, 85));
  }

  if (c.render_effect & static_cast<unsigned int>(CardEffect::Box)) {
    QPen pen1(QColor(255, 255, 0), 2);
    p.setPen(pen1);
    p.drawRect(1, 1, cw - 2, ch - 2);
  }

  return pix;
}

QRect HanafudaBoardWidget::_handCardRect(int idx) const {
  return QRect(10 + idx * 48, 400, 48, 78);
}

QRect HanafudaBoardWidget::_deskCardRect(int slot) const {
  return QRect(140 + (slot / 2) * 48, 100 + (slot & 1) * 78, 48, 78);
}

QRect HanafudaBoardWidget::_yesButtonRect() const {
  return QRect(350, 274, 125, 28);
}

QRect HanafudaBoardWidget::_noButtonRect() const {
  return QRect(485, 274, 125, 28);
}

void HanafudaBoardWidget::paintEvent(QPaintEvent* /*event*/) {
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing, true);

  // 1. Table felt background (matching SDLHana RGB 30, 130, 100)
  p.fillRect(rect(), QColor(30, 130, 100));
  p.setPen(QPen(QColor(0, 196, 196), 2));
  p.drawRect(1, 1, width() - 2, height() - 2);

  // Determine active hand card for desk-matching highlight
  int active_month = -1;
  if (_gameState.phase == GamePhase::SelectHandCard &&
      _gameState.current_player == 0) {
    int h_idx = (_hover_hand_idx >= 0 &&
                 _hover_hand_idx <
                     static_cast<int>(_gameState.players[0].hand.size()))
                    ? _hover_hand_idx
                    : _selected_hand_idx;
    if (h_idx >= 0 &&
        h_idx < static_cast<int>(_gameState.players[0].hand.size())) {
      active_month = _gameState.players[0].hand[h_idx].month();
    }
  }

  // 2. Draw Pile (x=50..58, y=95..103)
  int rem_deck = _gameState.remaining_deck_count();
  int pile_layers = std::clamp((rem_deck + 4) / 5, 1, 5);
  for (int i = 0; i < pile_layers; ++i) {
    p.drawPixmap(50 + i * 2, 95 + i * 2, renderCardPixmap(Card(255), 48, 78));
  }
  // Remaining deck count badge
  p.setPen(Qt::NoPen);
  p.setBrush(QColor(0, 0, 0, 170));
  p.drawRoundedRect(54, 152, 46, 20, 4, 4);
  p.setPen(QColor(255, 255, 180));
  QFont smallBold = font();
  smallBold.setPointSize(8);
  smallBold.setBold(true);
  p.setFont(smallBold);
  p.drawText(QRect(54, 152, 46, 20), Qt::AlignCenter,
             QString("%1 left").arg(rem_deck));

  // If in SelectDeskCardForDrawn or DoubleUp, show the drawn card next to pile
  if (_gameState.phase == GamePhase::SelectDeskCardForDrawn &&
      _gameState.last_drawn_card.is_valid()) {
    p.drawPixmap(86, 105, renderCardPixmap(_gameState.last_drawn_card, 48, 78));
  } else if ((_gameState.phase == GamePhase::AskDoubleUp ||
              _gameState.phase == GamePhase::AskBigOrSmall) &&
             _gameState.double_up_card.is_valid()) {
    p.drawPixmap(86, 105, renderCardPixmap(_gameState.double_up_card, 48, 78));
  }

  // 3. Score Box (x=20, y=190, w=112, h=70)
  p.setBrush(QColor(0, 155, 0, 180));
  p.setPen(QPen(QColor(220, 255, 220), 1));
  p.drawRoundedRect(20, 190, 112, 70, 5, 5);

  QFont scoreTitleFont = font();
  scoreTitleFont.setPointSize(11);
  scoreTitleFont.setBold(true);
  p.setFont(scoreTitleFont);
  p.setPen(QColor(255, 255, 0));
  p.drawText(QRect(24, 194, 104, 22), Qt::AlignCenter, "SCORE");

  QFont scoreValFont = font();
  scoreValFont.setPointSize(14);
  scoreValFont.setBold(true);
  p.setFont(scoreValFont);
  p.setPen(Qt::white);
  p.drawText(QRect(24, 216, 104, 26), Qt::AlignCenter,
             QString::number(_gameState.score));

  p.setFont(smallBold);
  p.setPen(QColor(200, 255, 200));
  p.drawText(
      QRect(24, 241, 104, 16), Qt::AlignCenter,
      QString("You:%1  Com:%2")
          .arg(_gameState.players[0].result.score)
          .arg(_gameState.players[1].result.score));

  // 4. Opponent (Bot) Hand Cards (top-left: x=10, y=10)
  int bot_hand_sz = static_cast<int>(_gameState.players[1].hand.size());
  for (int i = 0; i < bot_hand_sz; ++i) {
    // Reveal bot hand when round is over
    Card c = (_gameState.phase == GamePhase::RoundOver)
                 ? _gameState.players[1].hand[i]
                 : Card(255);
    p.drawPixmap(10 + i * 48, 10, renderCardPixmap(c, 48, 78));
  }

  // 5. Opponent Captured Cards (top-right, matching SDLHana DrawCaptured)
  auto bot_norm = _gameState.players[1].captured_normal();
  auto bot_spec = _gameState.players[1].captured_special();
  int bx = 588;
  for (int i = 0; i < static_cast<int>(bot_norm.size()); ++i) {
    int draw_x = (i < 9) ? (bx - 8 * (i + 1)) : (570 - 3 * (i - 9));
    p.drawPixmap(draw_x, 10, renderCardPixmap(bot_norm[i], 48, 78));
  }
  if (!bot_norm.empty()) {
    p.setBrush(QColor(0, 0, 0, 175));
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(592, 68, 40, 18, 3, 3);
    p.setPen(Qt::white);
    p.setFont(smallBold);
    p.drawText(QRect(592, 68, 40, 18), Qt::AlignCenter,
               QString("x%1").arg(bot_norm.size()));
  }

  if (!bot_spec.empty()) {
    int sx = 500 - 48;
    int avail_w = std::max(48, sx - 20 - 48 * bot_hand_sz);
    int per_w =
        std::min(48, avail_w / static_cast<int>(bot_spec.size()));
    for (int i = 0; i < static_cast<int>(bot_spec.size()); ++i) {
      p.drawPixmap(sx, 10, renderCardPixmap(bot_spec[i], 48, 78));
      sx -= per_w;
    }
  }

  // 6. Desk Cards (2x10 grid at x=140, y=100)
  for (int i = 0; i < 20; ++i) {
    QRect r = _deskCardRect(i);
    if (i < _gameState.num_desk_cards && _gameState.desk_cards[i].is_valid()) {
      const Card& dc = _gameState.desk_cards[i];
      p.drawPixmap(r.topLeft(), renderCardPixmap(dc, 48, 78));

      // Highlight matching month on desk when hovering/selecting hand card
      if (active_month != -1 && dc.month() == active_month) {
        p.setPen(QPen(QColor(0, 255, 255), 2));
        p.setBrush(Qt::NoBrush);
        p.drawRect(r.adjusted(1, 1, -2, -2));
      }

      // Highlight hovered desk slot during desk selection phase
      if ((_gameState.phase == GamePhase::SelectDeskCardForHand ||
           _gameState.phase == GamePhase::SelectDeskCardForDrawn) &&
          (i == _gameState.desk_choice_slots[0] ||
           i == _gameState.desk_choice_slots[1])) {
        QColor hc = (i == _hover_desk_slot) ? QColor(255, 80, 0)
                                            : QColor(255, 255, 0);
        p.setPen(QPen(hc, 3));
        p.setBrush(Qt::NoBrush);
        p.drawRect(r.adjusted(1, 1, -2, -2));
      }
    } else if (i < 8) {
      // Subtle empty slot outline for the core 8 table positions
      p.setPen(QPen(QColor(20, 105, 80), 1, Qt::DashLine));
      p.setBrush(Qt::NoBrush);
      p.drawRect(r.adjusted(2, 2, -3, -3));
    }
  }

  // 7. Status / Prompt Banner Bar (x=20, y=270, w=600, h=36)
  QColor bannerBg(40, 55, 85, 210);
  if (_gameState.phase == GamePhase::AskKoiKoi ||
      _gameState.phase == GamePhase::AskDoubleUp ||
      _gameState.phase == GamePhase::AskBigOrSmall) {
    bannerBg = QColor(95, 45, 95, 225);
  } else if (_gameState.phase == GamePhase::RoundOver) {
    bannerBg = (_gameState.winner == 0)
                   ? QColor(25, 95, 45, 225)
                   : ((_gameState.winner == 1) ? QColor(110, 35, 35, 225)
                                               : QColor(50, 65, 85, 225));
  }
  p.setBrush(bannerBg);
  p.setPen(QPen(QColor(210, 210, 230), 1));
  p.drawRoundedRect(20, 270, 600, 36, 5, 5);

  QFont bannerFont = font();
  bannerFont.setPointSize(10);
  bannerFont.setBold(true);
  p.setFont(bannerFont);
  p.setPen(QColor(255, 255, 120));

  bool show_yes_no = (_gameState.phase == GamePhase::AskKoiKoi ||
                      _gameState.phase == GamePhase::AskDoubleUp ||
                      _gameState.phase == GamePhase::AskBigOrSmall);
  bool show_next_btn = (_gameState.phase == GamePhase::RoundOver);

  int text_w = (show_yes_no) ? 320 : (show_next_btn ? 445 : 580);
  p.drawText(QRect(30, 270, text_w, 36), Qt::AlignVCenter | Qt::AlignLeft,
             QString::fromStdString(_gameState.status_banner));

  if (show_yes_no) {
    QString yes_txt = QString::fromStdString(_gameState.msg("yes"));
    QString no_txt = QString::fromStdString(_gameState.msg("no"));
    if (_gameState.phase == GamePhase::AskKoiKoi) {
      yes_txt = QString::fromStdString(_gameState.msg("go"));
      no_txt = QString::fromStdString(_gameState.msg("stop"));
    } else if (_gameState.phase == GamePhase::AskBigOrSmall) {
      yes_txt = QString::fromStdString(_gameState.msg("big"));
      no_txt = QString::fromStdString(_gameState.msg("small"));
    }

    QRect ry = _yesButtonRect();
    QRect rn = _noButtonRect();

    p.setBrush(QColor(40, 140, 70));
    p.setPen(QPen(Qt::white, 1));
    p.drawRoundedRect(ry, 4, 4);
    p.setPen(Qt::white);
    p.drawText(ry, Qt::AlignCenter, yes_txt);

    p.setBrush(QColor(150, 50, 50));
    p.setPen(QPen(Qt::white, 1));
    p.drawRoundedRect(rn, 4, 4);
    p.setPen(Qt::white);
    p.drawText(rn, Qt::AlignCenter, no_txt);
  } else if (show_next_btn) {
    QRect rn = _noButtonRect();
    p.setBrush(QColor(45, 115, 180));
    p.setPen(QPen(Qt::white, 1));
    p.drawRoundedRect(rn, 4, 4);
    p.setPen(Qt::white);
    p.drawText(rn, Qt::AlignCenter, "Next Round →");
  }

  // 8. Player Captured Special Cards (y=316, x=580向左, matching SDLHana DrawCaptured)
  auto plr_spec = _gameState.players[0].captured_special();
  if (!plr_spec.empty()) {
    int width_avail = 600;
    int x = 580;
    int per_w =
        std::min(48, width_avail / static_cast<int>(plr_spec.size()));
    for (int i = 0; i < static_cast<int>(plr_spec.size()); ++i) {
      p.drawPixmap(x, 314, renderCardPixmap(plr_spec[i], 48, 78));
      x -= per_w;
    }
  }

  // 9. Player Captured Normal / Chaff Cards (y=400, x=580向左)
  auto plr_norm = _gameState.players[0].captured_normal();
  for (int i = 0; i < static_cast<int>(plr_norm.size()); ++i) {
    int draw_x = (i < 9) ? (580 - 8 * i) : (570 - 3 * (i - 9));
    p.drawPixmap(draw_x, 400, renderCardPixmap(plr_norm[i], 48, 78));
  }
  if (!plr_norm.empty()) {
    p.setBrush(QColor(0, 0, 0, 175));
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(592, 458, 40, 18, 3, 3);
    p.setPen(Qt::white);
    p.setFont(smallBold);
    p.drawText(QRect(592, 458, 40, 18), Qt::AlignCenter,
               QString("x%1").arg(plr_norm.size()));
  }

  // 10. Player Hand Cards (bottom-left: x=10 + i*48, y=400)
  int plr_hand_sz = static_cast<int>(_gameState.players[0].hand.size());
  for (int i = 0; i < plr_hand_sz; ++i) {
    const Card& hc = _gameState.players[0].hand[i];
    bool is_hovered = (i == _hover_hand_idx);
    bool is_selected = (i == _selected_hand_idx);
    bool has_desk_match =
        !_gameState.matching_desk_slots(hc.month()).empty();

    int draw_x = 10 + i * 48;
    int draw_y = (is_hovered || is_selected) ? 394 : 400;

    p.drawPixmap(draw_x, draw_y, renderCardPixmap(hc, 48, 78));

    if (is_hovered || is_selected) {
      p.setPen(QPen(is_hovered ? QColor(255, 255, 0) : QColor(0, 220, 255), 2));
      p.setBrush(Qt::NoBrush);
      p.drawRect(draw_x + 1, draw_y + 1, 45, 75);
    }

    // Key number badge (1..8) and match indicator dot
    p.setBrush(QColor(0, 0, 0, 175));
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(draw_x + 2, draw_y + 2, 16, 15, 3, 3);
    p.setPen(has_desk_match ? QColor(100, 255, 100) : QColor(210, 210, 210));
    p.setFont(smallBold);
    p.drawText(QRect(draw_x + 2, draw_y + 2, 16, 15), Qt::AlignCenter,
               QString::number(i + 1));

    if (has_desk_match) {
      p.setBrush(QColor(50, 255, 80));
      p.setPen(QPen(Qt::black, 1));
      p.drawEllipse(draw_x + 36, draw_y + 4, 8, 8);
    }
  }
}

void HanafudaBoardWidget::mouseMoveEvent(QMouseEvent* event) {
  QPoint pos = event->pos();
  int new_hover_hand = -1;
  int new_hover_desk = -1;

  int plr_hand_sz = static_cast<int>(_gameState.players[0].hand.size());
  for (int i = 0; i < plr_hand_sz; ++i) {
    if (_handCardRect(i).adjusted(0, -6, 0, 0).contains(pos)) {
      new_hover_hand = i;
      const Card& c = _gameState.players[0].hand[i];
      auto matches = _gameState.matching_desk_slots(c.month());
      QString tip = QString("<b>%1</b><br>Type: %2<br>%3")
                        .arg(QString::fromStdString(card_name(c)))
                        .arg(QString::fromStdString(card_type_name(c.type())))
                        .arg(matches.empty()
                                 ? "No matching month on table"
                                 : QString("Matches %1 card(s) on table!")
                                       .arg(matches.size()));
      QToolTip::showText(event->globalPosition().toPoint(), tip, this);
      break;
    }
  }

  for (int i = 0; i < _gameState.num_desk_cards; ++i) {
    if (_gameState.desk_cards[i].is_valid() && _deskCardRect(i).contains(pos)) {
      new_hover_desk = i;
      const Card& c = _gameState.desk_cards[i];
      QString tip = QString("<b>%1</b><br>Type: %2")
                        .arg(QString::fromStdString(card_name(c)))
                        .arg(QString::fromStdString(card_type_name(c.type())));
      QToolTip::showText(event->globalPosition().toPoint(), tip, this);
      break;
    }
  }

  if (new_hover_hand != _hover_hand_idx || new_hover_desk != _hover_desk_slot) {
    _hover_hand_idx = new_hover_hand;
    _hover_desk_slot = new_hover_desk;
    if (_hover_hand_idx >= 0) {
      _selected_hand_idx = _hover_hand_idx;
    }
    update();
  }
}

void HanafudaBoardWidget::leaveEvent(QEvent* /*event*/) {
  _hover_hand_idx = -1;
  _hover_desk_slot = -1;
  update();
}

void HanafudaBoardWidget::mousePressEvent(QMouseEvent* event) {
  if (event->button() != Qt::LeftButton) {
    QWidget::mousePressEvent(event);
    return;
  }

  QPoint pos = event->pos();

  if (_gameState.phase == GamePhase::AskKoiKoi ||
      _gameState.phase == GamePhase::AskDoubleUp ||
      _gameState.phase == GamePhase::AskBigOrSmall) {
    if (_yesButtonRect().contains(pos)) {
      emit decisionClicked(true);
      return;
    }
    if (_noButtonRect().contains(pos)) {
      emit decisionClicked(false);
      return;
    }
  }

  if (_gameState.phase == GamePhase::RoundOver) {
    if (_noButtonRect().contains(pos) || QRect(20, 270, 600, 36).contains(pos)) {
      emit nextRoundClicked();
      return;
    }
  }

  if (_gameState.phase == GamePhase::SelectDeskCardForHand ||
      _gameState.phase == GamePhase::SelectDeskCardForDrawn) {
    for (int slot : _gameState.desk_choice_slots) {
      if (slot >= 0 && _deskCardRect(slot).contains(pos)) {
        emit deskCardClicked(slot);
        return;
      }
    }
  }

  if (_gameState.phase == GamePhase::SelectHandCard &&
      _gameState.current_player == 0) {
    int plr_hand_sz = static_cast<int>(_gameState.players[0].hand.size());
    for (int i = 0; i < plr_hand_sz; ++i) {
      if (_handCardRect(i).adjusted(0, -6, 0, 0).contains(pos)) {
        _selected_hand_idx = i;
        emit handCardClicked(i);
        return;
      }
    }
  }
}

// ============================================================================
// MainWindow Implementation
// ============================================================================

MainWindow::MainWindow(QWidget* parent) : QWidget(parent), _gameState() {
  _setupWidget();
  updateAllUi();
}

MainWindow::MainWindow(GameState game_state, QWidget* parent)
    : QWidget(parent), _gameState(std::move(game_state)) {
  _setupWidget();
  updateAllUi();
}

void MainWindow::_setupWidget() {
  setWindowTitle(QString::fromUtf8(kProgramName.data(), kProgramName.size()));
  setFocusPolicy(Qt::StrongFocus);

  _bot_timer = new QTimer(this);
  _bot_timer->setSingleShot(true);
  connect(_bot_timer, &QTimer::timeout, this, &MainWindow::onBotTimerTimeout);

  QVBoxLayout* vbox_main = new QVBoxLayout(this);
  vbox_main->setContentsMargins(5, 5, 5, 5);
  vbox_main->setSpacing(5);

  // Top Information / Game Log box (matching DruxLord layout)
  QGroupBox* frame_info = new QGroupBox("Information & Turn Log", this);
  QVBoxLayout* vbox_info = new QVBoxLayout(frame_info);
  vbox_info->setContentsMargins(5, 5, 5, 5);
  _textview_information = new QTextEdit(frame_info);
  _textview_information->setReadOnly(true);
  _textview_information->setFixedHeight(95);
  vbox_info->addWidget(_textview_information);
  vbox_main->addWidget(frame_info);

  QHBoxLayout* hbox_down = new QHBoxLayout();
  hbox_down->setSpacing(8);
  vbox_main->addLayout(hbox_down);

  // Left: Graphical Hanafuda Table
  QGroupBox* frame_table = new QGroupBox("Hanafuda Table", this);
  QVBoxLayout* vbox_table = new QVBoxLayout(frame_table);
  vbox_table->setContentsMargins(5, 5, 5, 5);
  _board_widget = new HanafudaBoardWidget(_gameState, frame_table);
  connect(_board_widget, &HanafudaBoardWidget::handCardClicked, this,
          &MainWindow::onHandCardClicked);
  connect(_board_widget, &HanafudaBoardWidget::deskCardClicked, this,
          &MainWindow::onDeskCardClicked);
  connect(_board_widget, &HanafudaBoardWidget::decisionClicked, this,
          &MainWindow::onBoardDecisionClicked);
  connect(_board_widget, &HanafudaBoardWidget::nextRoundClicked, this,
          &MainWindow::slotNextRound);
  vbox_table->addWidget(_board_widget);
  hbox_down->addWidget(frame_table);

  // Middle: Action, Decision, Game controls
  QVBoxLayout* vbox_middle = new QVBoxLayout();
  vbox_middle->setSpacing(5);
  hbox_down->addLayout(vbox_middle);

  QGroupBox* frame_action = new QGroupBox("Action", this);
  QVBoxLayout* box_action = new QVBoxLayout(frame_action);
  box_action->setContentsMargins(5, 5, 5, 5);
  box_action->setSpacing(3);

  _button_play = new QPushButton("&Play Card", frame_action);
  connect(_button_play, &QPushButton::clicked, this, &MainWindow::slotPlayCard);
  box_action->addWidget(_button_play);

  _button_next_round = new QPushButton("Next &Round", frame_action);
  connect(_button_next_round, &QPushButton::clicked, this,
          &MainWindow::slotNextRound);
  box_action->addWidget(_button_next_round);

  _button_settings = new QPushButton("&Settings...", frame_action);
  connect(_button_settings, &QPushButton::clicked, this,
          &MainWindow::slotSettings);
  box_action->addWidget(_button_settings);

  _button_yaku_help = new QPushButton("&Yaku && Rules...", frame_action);
  connect(_button_yaku_help, &QPushButton::clicked, this,
          &MainWindow::slotYakuHelp);
  box_action->addWidget(_button_yaku_help);

  _button_deck_info = new QPushButton("Deck &Info...", frame_action);
  connect(_button_deck_info, &QPushButton::clicked, this,
          &MainWindow::slotDeckInfo);
  box_action->addWidget(_button_deck_info);

  _button_history = new QPushButton("Score &History...", frame_action);
  connect(_button_history, &QPushButton::clicked, this,
          &MainWindow::slotHistory);
  box_action->addWidget(_button_history);

  vbox_middle->addWidget(frame_action);

  QGroupBox* frame_decision = new QGroupBox("Decision", this);
  QVBoxLayout* box_decision = new QVBoxLayout(frame_decision);
  box_decision->setContentsMargins(5, 5, 5, 5);
  box_decision->setSpacing(3);

  _button_yes = new QPushButton("&Yes / Koi-Koi", frame_decision);
  connect(_button_yes, &QPushButton::clicked, this,
          &MainWindow::slotDecisionYes);
  box_decision->addWidget(_button_yes);

  _button_no = new QPushButton("&No / Stop", frame_decision);
  connect(_button_no, &QPushButton::clicked, this, &MainWindow::slotDecisionNo);
  box_decision->addWidget(_button_no);

  vbox_middle->addWidget(frame_decision);

  QGroupBox* frame_game = new QGroupBox("Game", this);
  QVBoxLayout* box_game = new QVBoxLayout(frame_game);
  box_game->setContentsMargins(5, 5, 5, 5);
  box_game->setSpacing(3);

  _checkbutton_sound = new QCheckBox("Sou&nd", frame_game);
  _checkbutton_sound->setChecked(_gameState.sound_enabled);
  connect(_checkbutton_sound, &QCheckBox::toggled, this, [this](bool checked) {
    _gameState.sound_enabled = checked;
  });
  box_game->addWidget(_checkbutton_sound);

  _button_about = new QPushButton("&About", frame_game);
  connect(_button_about, &QPushButton::clicked, this, &MainWindow::slotAbout);
  box_game->addWidget(_button_about);

  _button_docs = new QPushButton("Docs", frame_game);
  connect(_button_docs, &QPushButton::clicked, this, &MainWindow::slotDocs);
  box_game->addWidget(_button_docs);

  _button_highscores = new QPushButton("High Scores", frame_game);
  connect(_button_highscores, &QPushButton::clicked, this,
          &MainWindow::slotHighscores);
  box_game->addWidget(_button_highscores);

  _button_newgame = new QPushButton("New &Game", frame_game);
  connect(_button_newgame, &QPushButton::clicked, this,
          &MainWindow::slotNewGame);
  box_game->addWidget(_button_newgame);

  vbox_middle->addWidget(frame_game);
  vbox_middle->addStretch();

  // Right: Captured & Yaku summary + Status & Chart
  QVBoxLayout* vbox_right = new QVBoxLayout();
  vbox_right->setSpacing(5);
  hbox_down->addLayout(vbox_right);

  QGroupBox* frame_yaku = new QGroupBox("Captured Counts & Active Yaku", this);
  QVBoxLayout* vbox_yaku = new QVBoxLayout(frame_yaku);
  vbox_yaku->setContentsMargins(5, 5, 5, 5);

  _treeview_yaku = new QTreeWidget(frame_yaku);
  _treeview_yaku->setRootIsDecorated(false);
  _treeview_yaku->setUniformRowHeights(true);
  _treeview_yaku->setColumnCount(3);
  _treeview_yaku->setHeaderLabels({"Category / Yaku", "You", "Com"});
  _treeview_yaku->setColumnWidth(0, 155);
  _treeview_yaku->setColumnWidth(1, 45);
  _treeview_yaku->setColumnWidth(2, 45);
  _treeview_yaku->setMinimumSize(265, 220);
  vbox_yaku->addWidget(_treeview_yaku);
  vbox_right->addWidget(frame_yaku);

  QGroupBox* frame_status = new QGroupBox("Status", this);
  QVBoxLayout* vbox_status = new QVBoxLayout(frame_status);
  vbox_status->setContentsMargins(5, 5, 5, 5);
  vbox_status->setSpacing(5);

  QGridLayout* grid_info = new QGridLayout();
  grid_info->setHorizontalSpacing(12);
  grid_info->setVerticalSpacing(4);

  grid_info->addWidget(new QLabel("Mode:", frame_status), 0, 0);
  _label_mode = new QLabel(frame_status);
  grid_info->addWidget(_label_mode, 0, 1);

  grid_info->addWidget(new QLabel("Round:", frame_status), 1, 0);
  _label_round = new QLabel(frame_status);
  grid_info->addWidget(_label_round, 1, 1);

  grid_info->addWidget(new QLabel("Dealer:", frame_status), 2, 0);
  _label_dealer = new QLabel(frame_status);
  grid_info->addWidget(_label_dealer, 2, 1);

  grid_info->addWidget(new QLabel("Record:", frame_status), 3, 0);
  _label_record = new QLabel(frame_status);
  grid_info->addWidget(_label_record, 3, 1);

  vbox_status->addLayout(grid_info);

  QHBoxLayout* box_score_chart = new QHBoxLayout();
  box_score_chart->setSpacing(10);

  QVBoxLayout* vbox_scores = new QVBoxLayout();
  vbox_scores->addWidget(new QLabel("Total Score:", frame_status));
  _label_score = new QLabel(frame_status);
  vbox_scores->addWidget(_label_score);
  vbox_scores->addWidget(new QLabel("Round Pts:", frame_status));
  _label_round_pts = new QLabel(frame_status);
  vbox_scores->addWidget(_label_round_pts);
  vbox_scores->addStretch();
  box_score_chart->addLayout(vbox_scores);

  _drawingarea_status = new ScoreChartView(true, frame_status);
  _drawingarea_status->setFixedSize(145, 95);
  connect(_drawingarea_status, &ScoreChartView::itemChanged, this,
          [this](int) { _drawingarea_status->updateChart(_gameState); });
  connect(_drawingarea_status, &ScoreChartView::zoomRequested, this,
          &MainWindow::onStatusZoomRequested);
  box_score_chart->addWidget(_drawingarea_status);

  vbox_status->addLayout(box_score_chart);
  vbox_right->addWidget(frame_status);

  _shortcut_quit = new QShortcut(QKeySequence::Quit, this);
  connect(_shortcut_quit, &QShortcut::activated, this, &QWidget::close);

  layout()->setSizeConstraint(QLayout::SetFixedSize);
}

void MainWindow::updateAllUi() {
  if (_board_widget) {
    _board_widget->setSelectedHandIndex(_board_widget->selectedHandIndex());
    _board_widget->update();
  }

  if (_checkbutton_sound) {
    _checkbutton_sound->setChecked(_gameState.sound_enabled);
  }

  if (_label_mode) {
    _label_mode->setText(QString("<b>%1</b>").arg(QString::fromStdString(
        game_mode_name(_gameState.mode, _gameState.language))));
  }
  if (_label_round) {
    _label_round->setText(QString("<b>#%1</b> (Deck: %2)")
                              .arg(_gameState.round_number)
                              .arg(_gameState.remaining_deck_count()));
  }
  if (_label_dealer) {
    _label_dealer->setText(
        QString("<b>%1</b>")
            .arg(_gameState.dealer == 0 ? "You" : "Computer"));
  }
  if (_label_record) {
    _label_record->setText(QString("<b>%1W - %2L - %3D</b>")
                               .arg(_gameState.rounds_won)
                               .arg(_gameState.rounds_lost)
                               .arg(_gameState.rounds_drawn));
  }
  if (_label_score) {
    QString color = (_gameState.score >= 0) ? "#008800" : "#CC0000";
    _label_score->setText(
        QString("<span style=\"color:%1;font-size:13pt;\"><b>%2</b></span>")
            .arg(color)
            .arg(_gameState.score));
  }
  if (_label_round_pts) {
    _label_round_pts->setText(QString("<b>You: %1 | Com: %2</b>")
                                  .arg(_gameState.players[0].result.score)
                                  .arg(_gameState.players[1].result.score));
  }

  // Update button states
  bool can_play = (_gameState.phase == GamePhase::SelectHandCard &&
                   _gameState.current_player == 0);
  bool is_round_over = (_gameState.phase == GamePhase::RoundOver);
  bool is_decision = (_gameState.phase == GamePhase::AskKoiKoi ||
                      _gameState.phase == GamePhase::AskDoubleUp ||
                      _gameState.phase == GamePhase::AskBigOrSmall ||
                      _gameState.phase == GamePhase::SelectDeskCardForHand ||
                      _gameState.phase == GamePhase::SelectDeskCardForDrawn);

  if (_button_play) _button_play->setEnabled(can_play);
  if (_button_next_round) _button_next_round->setEnabled(is_round_over);
  if (_button_yes) {
    _button_yes->setEnabled(is_decision);
    if (_gameState.phase == GamePhase::AskBigOrSmall) {
      _button_yes->setText("&Big (Jul-Dec)");
    } else if (_gameState.phase == GamePhase::AskDoubleUp) {
      _button_yes->setText("&Yes (Double-Up)");
    } else if (_gameState.phase == GamePhase::SelectDeskCardForHand ||
               _gameState.phase == GamePhase::SelectDeskCardForDrawn) {
      _button_yes->setText("Pick &1st Match");
    } else {
      _button_yes->setText("&Yes (Koi-Koi)");
    }
  }
  if (_button_no) {
    _button_no->setEnabled(is_decision);
    if (_gameState.phase == GamePhase::AskBigOrSmall) {
      _button_no->setText("&Small (Jan-Jun)");
    } else if (_gameState.phase == GamePhase::AskDoubleUp) {
      _button_no->setText("&No (Keep Score)");
    } else if (_gameState.phase == GamePhase::SelectDeskCardForHand ||
               _gameState.phase == GamePhase::SelectDeskCardForDrawn) {
      _button_no->setText("Pick &2nd Match");
    } else {
      _button_no->setText("&No (Stop)");
    }
  }

  _fillTreeviewYaku();

  if (_drawingarea_status) {
    _drawingarea_status->updateChart(_gameState);
  }

  if (_textview_information) {
    QString log_text;
    int start = std::max(0, static_cast<int>(_gameState.game_log.size()) - 25);
    for (int i = start; i < static_cast<int>(_gameState.game_log.size()); ++i) {
      log_text += QString::fromStdString(_gameState.game_log[i]) + "\n";
    }
    _textview_information->setPlainText(log_text);
    auto cursor = _textview_information->textCursor();
    cursor.movePosition(QTextCursor::End);
    _textview_information->setTextCursor(cursor);
  }

  _scheduleBotIfNeeded();
}

void MainWindow::_fillTreeviewYaku() {
  if (!_treeview_yaku) return;
  _treeview_yaku->clear();

  const auto& p0 = _gameState.players[0];
  const auto& p1 = _gameState.players[1];

  auto add_row = [&](const QString& label, const QString& v0, const QString& v1,
                     bool bold = false) {
    auto* item = new QTreeWidgetItem(_treeview_yaku);
    item->setText(0, label);
    item->setText(1, v0);
    item->setText(2, v1);
    item->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
    item->setTextAlignment(2, Qt::AlignRight | Qt::AlignVCenter);
    if (bold) {
      QFont f = item->font(0);
      f.setBold(true);
      item->setFont(0, f);
      item->setFont(1, f);
      item->setFont(2, f);
    }
  };

  add_row("★ Lights (Gwang)",
          QString("%1/5").arg(p0.count_lights()),
          QString("%1/5").arg(p1.count_lights()));
  add_row("◆ Animals (Tane)",
          QString("%1/5").arg(p0.count_animals()),
          QString("%1/5").arg(p1.count_animals()));
  add_row("▬ Ribbons (Tanzaku)",
          QString("%1/5").arg(p0.count_ribbons()),
          QString("%1/5").arg(p1.count_ribbons()));
  add_row("· Chaff (Kasu)",
          QString("%1/10").arg(p0.count_chaff(_gameState.mode)),
          QString("%1/10").arg(p1.count_chaff(_gameState.mode)));

  if (p0.num_continue > 0 || p1.num_continue > 0) {
    add_row("Koi-Koi / Go Calls", QString::number(p0.num_continue),
            QString::number(p1.num_continue), true);
  }

  auto y0 = _gameState.get_yaku_items(0, false);
  auto y1 = _gameState.get_yaku_items(1, false);

  for (const auto& y : y0) {
    add_row(QString("✓ %1").arg(QString::fromStdString(y.name)),
            QString("%1p").arg(y.points), "-", true);
  }
  for (const auto& y : y1) {
    add_row(QString("⚠ %1").arg(QString::fromStdString(y.name)), "-",
            QString("%1p").arg(y.points), true);
  }
}

void MainWindow::_scheduleBotIfNeeded() {
  if (_gameState.phase == GamePhase::SelectHandCard &&
      _gameState.current_player == 1) {
    if (!_bot_timer->isActive()) {
      int delay = std::max(60, anim_speed_ms(_gameState.anim_speed));
      _bot_timer->start(delay);
    }
  }
}

void MainWindow::_playFeedbackSound() {
  if (_gameState.sound_enabled) {
    QApplication::beep();
  }
}

void MainWindow::onBotTimerTimeout() {
  if (_gameState.phase == GamePhase::SelectHandCard &&
      _gameState.current_player == 1) {
    _gameState.step_bot();
    _playFeedbackSound();
    updateAllUi();
  }
}

void MainWindow::onHandCardClicked(int hand_idx) {
  if (_gameState.phase == GamePhase::SelectHandCard &&
      _gameState.current_player == 0) {
    if (_gameState.play_hand_card(hand_idx)) {
      _playFeedbackSound();
      updateAllUi();
    }
  }
}

void MainWindow::onDeskCardClicked(int desk_slot) {
  if (_gameState.select_desk_card(desk_slot)) {
    _playFeedbackSound();
    updateAllUi();
  }
}

void MainWindow::onBoardDecisionClicked(bool yes_or_big) {
  if (yes_or_big) {
    decisionYes();
  } else {
    decisionNo();
  }
}

void MainWindow::onStatusZoomRequested(int item_idx) { showHistory(item_idx); }

void MainWindow::keyPressEvent(QKeyEvent* event) {
  int key = event->key();

  if (key >= Qt::Key_1 && key <= Qt::Key_8) {
    int idx = key - Qt::Key_1;
    if (_gameState.phase == GamePhase::SelectHandCard &&
        _gameState.current_player == 0 &&
        idx < static_cast<int>(_gameState.players[0].hand.size())) {
      if (_board_widget) _board_widget->setSelectedHandIndex(idx);
      onHandCardClicked(idx);
      return;
    } else if ((_gameState.phase == GamePhase::SelectDeskCardForHand ||
                _gameState.phase == GamePhase::SelectDeskCardForDrawn) &&
               (idx == 0 || idx == 1)) {
      onDeskCardClicked(_gameState.desk_choice_slots[idx]);
      return;
    }
  }

  if (key == Qt::Key_Left && _board_widget) {
    _board_widget->setSelectedHandIndex(_board_widget->selectedHandIndex() - 1);
    return;
  }
  if (key == Qt::Key_Right && _board_widget) {
    _board_widget->setSelectedHandIndex(_board_widget->selectedHandIndex() + 1);
    return;
  }
  if (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Space) {
    if (_gameState.phase == GamePhase::RoundOver) {
      slotNextRound();
      return;
    } else if (_gameState.phase == GamePhase::SelectHandCard &&
               _gameState.current_player == 0) {
      slotPlayCard();
      return;
    }
  }
  if (key == Qt::Key_Y) {
    slotDecisionYes();
    return;
  }
  if (key == Qt::Key_N) {
    slotDecisionNo();
    return;
  }

  QWidget::keyPressEvent(event);
}

// MainWindow control methods
void MainWindow::playSelectedCard() { window_main_button_play_clicked_cb(*this); }
void MainWindow::nextRound() { window_main_button_next_round_clicked_cb(*this); }
void MainWindow::decisionYes() { window_main_button_yes_clicked_cb(*this); }
void MainWindow::decisionNo() { window_main_button_no_clicked_cb(*this); }
void MainWindow::showSettings() { window_main_button_settings_clicked_cb(*this); }
void MainWindow::showYakuHelp() { window_main_button_yaku_help_clicked_cb(*this); }
void MainWindow::showDeckInfo() { window_main_button_deck_info_clicked_cb(*this); }
void MainWindow::showHistory(int item_idx) {
  WindowHistory dlg(_gameState, item_idx, this);
  dlg.exec();
}
void MainWindow::showAbout() { window_main_button_about_clicked_cb(*this); }
void MainWindow::showDocs() { window_main_button_docs_clicked_cb(*this); }
void MainWindow::showHighscores() {
  window_main_button_highscores_clicked_cb(*this);
}
void MainWindow::newGame() { window_main_button_newgame_clicked_cb(*this); }

// MainWindow slots
void MainWindow::slotPlayCard() { playSelectedCard(); }
void MainWindow::slotNextRound() { nextRound(); }
void MainWindow::slotDecisionYes() { decisionYes(); }
void MainWindow::slotDecisionNo() { decisionNo(); }
void MainWindow::slotSettings() { showSettings(); }
void MainWindow::slotYakuHelp() { showYakuHelp(); }
void MainWindow::slotDeckInfo() { showDeckInfo(); }
void MainWindow::slotHistory() { window_main_button_history_clicked_cb(*this); }
void MainWindow::slotAbout() { showAbout(); }
void MainWindow::slotDocs() { showDocs(); }
void MainWindow::slotHighscores() { showHighscores(); }
void MainWindow::slotNewGame() { newGame(); }

// ============================================================================
// WindowSettings Implementation
// ============================================================================

WindowSettings::WindowSettings(GameState& gameState, QWidget* parent)
    : QDialog(parent), _gameState(gameState) {
  _setupWidget();
}

void WindowSettings::_setupWidget() {
  setWindowTitle("Game Settings");
  setModal(true);

  QVBoxLayout* vbox = new QVBoxLayout(this);
  QFormLayout* form = new QFormLayout();

  _combo_mode = new QComboBox(this);
  _combo_mode->addItem("Koi-Koi (Japanese 8-Card)",
                       static_cast<int>(GameMode::KoiKoi));
  _combo_mode->addItem("Koi-Koi [BET] (6-Card + Double-Up)",
                       static_cast<int>(GameMode::Bet));
  _combo_mode->addItem("Go-Stop (Korean Rules)",
                       static_cast<int>(GameMode::Korean));
  _combo_mode->setCurrentIndex(static_cast<int>(_gameState.mode));
  form->addRow("Game Mode:", _combo_mode);

  _combo_language = new QComboBox(this);
  _combo_language->addItem("English", static_cast<int>(Language::English));
  _combo_language->addItem("Français", static_cast<int>(Language::French));
  _combo_language->addItem("日本語", static_cast<int>(Language::Japanese));
  _combo_language->setCurrentIndex(static_cast<int>(_gameState.language));
  form->addRow("Language:", _combo_language);

  _combo_speed = new QComboBox(this);
  _combo_speed->addItem("Very Slow", static_cast<int>(AnimSpeed::VerySlow));
  _combo_speed->addItem("Slow", static_cast<int>(AnimSpeed::Slow));
  _combo_speed->addItem("Middle", static_cast<int>(AnimSpeed::Middle));
  _combo_speed->addItem("Fast", static_cast<int>(AnimSpeed::Fast));
  _combo_speed->addItem("Very Fast", static_cast<int>(AnimSpeed::VeryFast));
  _combo_speed->setCurrentIndex(static_cast<int>(_gameState.anim_speed));
  form->addRow("Game Speed:", _combo_speed);

  _check_sound = new QCheckBox("Enable Sound Effects", this);
  _check_sound->setChecked(_gameState.sound_enabled);
  form->addRow("Audio:", _check_sound);

  vbox->addLayout(form);

  QHBoxLayout* hbox_btns = new QHBoxLayout();
  hbox_btns->addStretch();
  _button_ok = new QPushButton("&OK", this);
  _button_cancel = new QPushButton("&Cancel", this);
  connect(_button_ok, &QPushButton::clicked, this,
          &WindowSettings::onOkClicked);
  connect(_button_cancel, &QPushButton::clicked, this, &QDialog::reject);
  hbox_btns->addWidget(_button_ok);
  hbox_btns->addWidget(_button_cancel);
  vbox->addLayout(hbox_btns);
}

void WindowSettings::onOkClicked() {
  GameMode new_mode =
      static_cast<GameMode>(_combo_mode->currentData().toInt());
  bool mode_changed = (new_mode != _gameState.mode);

  _gameState.mode = new_mode;
  _gameState.language =
      static_cast<Language>(_combo_language->currentData().toInt());
  _gameState.anim_speed =
      static_cast<AnimSpeed>(_combo_speed->currentData().toInt());
  _gameState.sound_enabled = _check_sound->isChecked();

  emit settingsApplied(mode_changed);
  accept();
}

// ============================================================================
// WindowYakuHelp Implementation
// ============================================================================

WindowYakuHelp::WindowYakuHelp(const GameState& gameState,
                               const HanafudaBoardWidget* boardWidget,
                               QWidget* parent)
    : QDialog(parent), _gameState(gameState) {
  _setupWidget(boardWidget);
}

void WindowYakuHelp::_setupWidget(const HanafudaBoardWidget* boardWidget) {
  setWindowTitle("Hanafuda Yaku & Scoring Reference");
  resize(680, 520);

  QVBoxLayout* vbox = new QVBoxLayout(this);

  QTreeWidget* tree = new QTreeWidget(this);
  tree->setRootIsDecorated(false);
  tree->setIconSize(QSize(32, 52));
  tree->setColumnCount(4);
  tree->setHeaderLabels(
      {"Yaku Combination", "Koi-Koi Pts", "Go-Stop Pts", "Required Cards"});
  tree->setColumnWidth(0, 175);
  tree->setColumnWidth(1, 90);
  tree->setColumnWidth(2, 90);
  tree->setColumnWidth(3, 280);

  struct YakuRef {
    const char* name;
    const char* jpn_pts;
    const char* kor_pts;
    const char* desc;
    std::uint8_t sample_card;
  };

  constexpr std::array<YakuRef, 13> kRefs = {{
      {"Five Lights (Gokō)", "15 pts", "15 pts",
       "All 5 Light cards (Jan, Mar, Aug, Nov, Dec)", 0},
      {"Four Lights (Shikō)", "10 pts", "4 pts",
       "4 Light cards excluding Rain Man (Nov)", 44},
      {"Rain Four Lights (Ame-Shikō)", "8 pts", "4 pts",
       "4 Light cards including Rain Man (Nov)", 40},
      {"Three Lights (Sankō)", "6 pts", "3 pts",
       "Any 3 Light cards excluding Rain Man", 8},
      {"Rain Three Lights", "—", "2 pts",
       "3 Light cards including Rain Man (Go-Stop only)", 40},
      {"Boar, Deer & Butterfly", "5 pts", "—",
       "Jun Butterflies + Jul Boar + Oct Deer", 24},
      {"Five Birds (Godori)", "—", "5 pts",
       "Feb Warbler + Apr Cuckoo + Aug Geese (Go-Stop)", 4},
      {"Flower Meets Sake Cup", "3 pts", "—",
       "Mar Cherry Curtain + Sep Sake Cup (Koi-Koi)", 32},
      {"Moon Meets Sake Cup", "3 pts", "—",
       "Aug Full Moon + Sep Sake Cup (Koi-Koi)", 28},
      {"Red Poetry Ribbons (Akatan)", "6 pts", "3 pts",
       "Jan, Feb, Mar Poetry Ribbons", 1},
      {"Blue Ribbons (Aotan)", "6 pts", "3 pts",
       "Jun, Sep, Oct Blue Ribbons", 21},
      {"Grass Ribbons (Chodan)", "—", "3 pts",
       "Apr, May, Jul Plain Red Ribbons (Go-Stop)", 13},
      {"Animals (5+) / Ribbons (5+) / Chaff (10+)", "1 pt + 1/extra",
       "1 pt + 1/extra", "5+ Animals, 5+ Ribbons, or 10+ Chaff cards", 16},
  }};

  for (const auto& r : kRefs) {
    auto* item = new QTreeWidgetItem(tree);
    if (boardWidget) {
      item->setIcon(0, QIcon(boardWidget->renderCardPixmap(Card(r.sample_card),
                                                           32, 52)));
    }
    item->setText(0, r.name);
    item->setText(1, r.jpn_pts);
    item->setText(2, r.kor_pts);
    item->setText(3, r.desc);
  }

  vbox->addWidget(tree);

  QPushButton* btn_close = new QPushButton("&Close", this);
  connect(btn_close, &QPushButton::clicked, this, &QDialog::accept);
  QHBoxLayout* hbox = new QHBoxLayout();
  hbox->addStretch();
  hbox->addWidget(btn_close);
  vbox->addLayout(hbox);
}

// ============================================================================
// WindowDeckInfo Implementation
// ============================================================================

WindowDeckInfo::WindowDeckInfo(const GameState& gameState,
                               const HanafudaBoardWidget* boardWidget,
                               QWidget* parent)
    : QDialog(parent), _gameState(gameState) {
  _setupWidget(boardWidget);
}

QString WindowDeckInfo::_cardLocationStatus(std::uint8_t card_id) const {
  if (_gameState.players[0].has_captured(card_id)) {
    return "Captured (You)";
  }
  if (_gameState.players[1].has_captured(card_id)) {
    return "Captured (Computer)";
  }
  for (int i = 0; i < _gameState.num_desk_cards; ++i) {
    if (_gameState.desk_cards[i].is_valid() &&
        _gameState.desk_cards[i].id() == card_id) {
      return "On Table";
    }
  }
  for (const auto& c : _gameState.players[0].hand) {
    if (c.id() == card_id) {
      return "In Your Hand";
    }
  }
  return "Unseen (Deck / Opponent)";
}

void WindowDeckInfo::_setupWidget(const HanafudaBoardWidget* boardWidget) {
  setWindowTitle("48-Card Hanafuda Deck Inspector");
  resize(650, 520);

  QVBoxLayout* vbox = new QVBoxLayout(this);

  QTreeWidget* tree = new QTreeWidget(this);
  tree->setRootIsDecorated(false);
  tree->setIconSize(QSize(28, 45));
  tree->setColumnCount(4);
  tree->setHeaderLabels({"Card", "Month", "Category", "Current Location"});
  tree->setColumnWidth(0, 230);
  tree->setColumnWidth(1, 110);
  tree->setColumnWidth(2, 110);
  tree->setColumnWidth(3, 160);

  for (int id = 0; id < 48; ++id) {
    Card c(static_cast<std::uint8_t>(id));
    auto* item = new QTreeWidgetItem(tree);
    if (boardWidget) {
      item->setIcon(0, QIcon(boardWidget->renderCardPixmap(c, 28, 45)));
    }
    item->setText(0, QString::fromStdString(card_name(c)));
    item->setText(1, QString::fromStdString(month_name(c.month())));
    item->setText(2, QString::fromStdString(card_type_name(c.type())));
    QString loc = _cardLocationStatus(c.id());
    item->setText(3, loc);
  }

  vbox->addWidget(tree);

  QPushButton* btn_close = new QPushButton("&Close", this);
  connect(btn_close, &QPushButton::clicked, this, &QDialog::accept);
  QHBoxLayout* hbox = new QHBoxLayout();
  hbox->addStretch();
  hbox->addWidget(btn_close);
  vbox->addLayout(hbox);
}

// ============================================================================
// WindowHistory Implementation
// ============================================================================

WindowHistory::WindowHistory(const GameState& gameState, int initial_item,
                             QWidget* parent)
    : QDialog(parent), _gameState(gameState) {
  _setupWidget(initial_item);
}

void WindowHistory::_setupWidget(int initial_item) {
  setWindowTitle("Score & Round History");
  resize(620, 420);

  QVBoxLayout* vbox = new QVBoxLayout(this);

  QHBoxLayout* top_bar = new QHBoxLayout();
  top_bar->addWidget(new QLabel("Series:", this));
  _combo_item = new QComboBox(this);
  for (int i = 0; i < ScoreChartView::TOTAL_ITEMS; ++i) {
    _combo_item->addItem(ScoreChartView::itemName(i), i);
  }
  _combo_item->setCurrentIndex(
      std::clamp(initial_item, 0, ScoreChartView::TOTAL_ITEMS - 1));
  connect(_combo_item, &QComboBox::currentIndexChanged, this,
          &WindowHistory::onItemChanged);
  top_bar->addWidget(_combo_item);
  top_bar->addStretch();
  vbox->addLayout(top_bar);

  _chart_view = new ScoreChartView(false, this);
  _chart_view->setItemIndex(_combo_item->currentIndex());
  _chart_view->updateChart(_gameState);
  vbox->addWidget(_chart_view, 1);

  QHBoxLayout* bottom_bar = new QHBoxLayout();
  bottom_bar->addStretch();
  _button_close = new QPushButton("&Close", this);
  connect(_button_close, &QPushButton::clicked, this, &QDialog::accept);
  bottom_bar->addWidget(_button_close);
  vbox->addLayout(bottom_bar);
}

void WindowHistory::onItemChanged(int index) {
  if (_chart_view) {
    _chart_view->setItemIndex(index);
    _refreshChart();
  }
}

void WindowHistory::_refreshChart() {
  if (_chart_view) {
    _chart_view->updateChart(_gameState);
  }
}
