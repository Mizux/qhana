#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QDialog>
#include <QGroupBox>
#include <QImage>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPixmap>
#include <QPushButton>
#include <QShortcut>
#include <QString>
#include <QTextEdit>
#include <QTimer>
#include <QTreeWidget>
#include <QWidget>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QValueAxis>

#include "qhana.h"

class ScoreChartView : public QChartView {
  Q_OBJECT

 public:
  static constexpr int ITEM_TOTAL_SCORE = 0;
  static constexpr int ITEM_PLAYER_ROUND = 1;
  static constexpr int ITEM_BOT_ROUND = 2;
  static constexpr int TOTAL_ITEMS = 3;

  explicit ScoreChartView(bool compact = true, QWidget* parent = nullptr);
  virtual ~ScoreChartView() = default;

  ScoreChartView(const ScoreChartView&) = delete;
  ScoreChartView& operator=(const ScoreChartView&) = delete;

  int itemIndex() const { return _item_idx; }
  void setItemIndex(int idx);

  void updateChart(const GameState& gameState);
  static QString itemName(int item_idx);

 signals:
  void itemChanged(int item_idx);
  void zoomRequested(int item_idx);

 protected:
  void mousePressEvent(QMouseEvent* event) override;
  void contextMenuEvent(QContextMenuEvent* event) override;

 private:
  void _setupChart();

  bool _compact = true;
  int _item_idx = ITEM_TOTAL_SCORE;

  QChart* _chart = nullptr;
  QValueAxis* _axis_x = nullptr;
  QValueAxis* _axis_y = nullptr;
  QLineSeries* _series_zero = nullptr;
  QLineSeries* _series_data = nullptr;
  QScatterSeries* _series_points = nullptr;
};

class HanafudaBoardWidget : public QWidget {
  Q_OBJECT

 public:
  explicit HanafudaBoardWidget(GameState& gameState, QWidget* parent = nullptr);
  virtual ~HanafudaBoardWidget() = default;

  HanafudaBoardWidget(const HanafudaBoardWidget&) = delete;
  HanafudaBoardWidget& operator=(const HanafudaBoardWidget&) = delete;

  int selectedHandIndex() const { return _selected_hand_idx; }
  void setSelectedHandIndex(int idx);

  QPixmap renderCardPixmap(const Card& c, int w = 48, int h = 78) const;

 signals:
  void handCardClicked(int hand_idx);
  void deskCardClicked(int desk_slot);
  void decisionClicked(bool yes_or_big);
  void nextRoundClicked();

 protected:
  void paintEvent(QPaintEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void leaveEvent(QEvent* event) override;

 private:
  void _loadAssets();
  QRect _handCardRect(int idx) const;
  QRect _deskCardRect(int slot) const;
  QRect _yesButtonRect() const;
  QRect _noButtonRect() const;

  GameState& _gameState;
  QImage _cards_img;
  QImage _back_img;
  int _hover_hand_idx = -1;
  int _hover_desk_slot = -1;
  int _selected_hand_idx = 0;
};

class MainWindow : public QWidget {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget* parent = nullptr);
  explicit MainWindow(GameState game_state, QWidget* parent = nullptr);
  virtual ~MainWindow() = default;

  MainWindow(const MainWindow&) = delete;
  MainWindow& operator=(const MainWindow&) = delete;

  GameState& gameState() { return _gameState; }
  const GameState& gameState() const { return _gameState; }

  void updateAllUi();

  HanafudaBoardWidget* boardWidget() const { return _board_widget; }
  QTreeWidget* treeviewYaku() const { return _treeview_yaku; }
  QTextEdit* textviewInformation() const { return _textview_information; }
  ScoreChartView* statusChartView() const { return _drawingarea_status; }

  // Controlling actions & dialogs
  void playSelectedCard();
  void nextRound();
  void decisionYes();
  void decisionNo();
  void showSettings();
  void showYakuHelp();
  void showDeckInfo();
  void showHistory(int item_idx = 0);
  void showAbout();
  void showDocs();
  void showHighscores();
  void newGame();

 signals:
  void stateUpdated();

 public slots:
  void slotPlayCard();
  void slotNextRound();
  void slotDecisionYes();
  void slotDecisionNo();
  void slotSettings();
  void slotYakuHelp();
  void slotDeckInfo();
  void slotHistory();
  void slotAbout();
  void slotDocs();
  void slotHighscores();
  void slotNewGame();
  void onHandCardClicked(int hand_idx);
  void onDeskCardClicked(int desk_slot);
  void onBoardDecisionClicked(bool yes_or_big);
  void onBotTimerTimeout();
  void onStatusZoomRequested(int item_idx);

 protected:
  void keyPressEvent(QKeyEvent* event) override;

 private:
  void _setupWidget();
  void _fillTreeviewYaku();
  void _scheduleBotIfNeeded();
  void _playFeedbackSound();

  GameState _gameState;

  HanafudaBoardWidget* _board_widget = nullptr;
  QTextEdit* _textview_information = nullptr;
  QTreeWidget* _treeview_yaku = nullptr;

  QPushButton* _button_play = nullptr;
  QPushButton* _button_next_round = nullptr;
  QPushButton* _button_yes = nullptr;
  QPushButton* _button_no = nullptr;
  QPushButton* _button_settings = nullptr;
  QPushButton* _button_yaku_help = nullptr;
  QPushButton* _button_deck_info = nullptr;
  QPushButton* _button_history = nullptr;
  QPushButton* _button_about = nullptr;
  QPushButton* _button_docs = nullptr;
  QPushButton* _button_highscores = nullptr;
  QPushButton* _button_newgame = nullptr;
  QCheckBox* _checkbutton_sound = nullptr;

  QLabel* _label_mode = nullptr;
  QLabel* _label_round = nullptr;
  QLabel* _label_dealer = nullptr;
  QLabel* _label_score = nullptr;
  QLabel* _label_record = nullptr;
  QLabel* _label_round_pts = nullptr;
  ScoreChartView* _drawingarea_status = nullptr;

  QTimer* _bot_timer = nullptr;
  QShortcut* _shortcut_quit = nullptr;
};

class WindowSettings : public QDialog {
  Q_OBJECT

 public:
  explicit WindowSettings(GameState& gameState, QWidget* parent = nullptr);
  virtual ~WindowSettings() = default;

  WindowSettings(const WindowSettings&) = delete;
  WindowSettings& operator=(const WindowSettings&) = delete;

 signals:
  void settingsApplied(bool mode_changed);

 private slots:
  void onOkClicked();

 private:
  void _setupWidget();

  GameState& _gameState;
  QComboBox* _combo_mode = nullptr;
  QComboBox* _combo_language = nullptr;
  QComboBox* _combo_speed = nullptr;
  QCheckBox* _check_sound = nullptr;
  QPushButton* _button_ok = nullptr;
  QPushButton* _button_cancel = nullptr;
};

class WindowYakuHelp : public QDialog {
  Q_OBJECT

 public:
  explicit WindowYakuHelp(const GameState& gameState,
                          const HanafudaBoardWidget* boardWidget,
                          QWidget* parent = nullptr);
  virtual ~WindowYakuHelp() = default;

  WindowYakuHelp(const WindowYakuHelp&) = delete;
  WindowYakuHelp& operator=(const WindowYakuHelp&) = delete;

 private:
  void _setupWidget(const HanafudaBoardWidget* boardWidget);

  const GameState& _gameState;
};

class WindowDeckInfo : public QDialog {
  Q_OBJECT

 public:
  explicit WindowDeckInfo(const GameState& gameState,
                          const HanafudaBoardWidget* boardWidget,
                          QWidget* parent = nullptr);
  virtual ~WindowDeckInfo() = default;

  WindowDeckInfo(const WindowDeckInfo&) = delete;
  WindowDeckInfo& operator=(const WindowDeckInfo&) = delete;

 private:
  void _setupWidget(const HanafudaBoardWidget* boardWidget);
  QString _cardLocationStatus(std::uint8_t card_id) const;

  const GameState& _gameState;
};

class WindowHistory : public QDialog {
  Q_OBJECT

 public:
  explicit WindowHistory(const GameState& gameState, int initial_item = 0,
                         QWidget* parent = nullptr);
  virtual ~WindowHistory() = default;

  WindowHistory(const WindowHistory&) = delete;
  WindowHistory& operator=(const WindowHistory&) = delete;

 public slots:
  void onItemChanged(int index);

 private:
  void _setupWidget(int initial_item);
  void _refreshChart();

  const GameState& _gameState;
  QComboBox* _combo_item = nullptr;
  ScoreChartView* _chart_view = nullptr;
  QPushButton* _button_close = nullptr;
};
