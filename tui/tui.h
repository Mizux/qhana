#pragma once

#include <string>
#include <vector>

#include "qhana.h"

class TuiApp {
 public:
  static constexpr int ITEM_TOTAL_SCORE = 0;
  static constexpr int ITEM_PLAYER_ROUND = 1;
  static constexpr int ITEM_BOT_ROUND = 2;
  static constexpr int TOTAL_ITEMS = 3;

  TuiApp();
  ~TuiApp();

  TuiApp(const TuiApp&) = delete;
  TuiApp& operator=(const TuiApp&) = delete;

  int run();

 private:
  enum class FocusPane { Hand, Desk };

  // Rendering helpers
  void drawDashboard();
  void drawTopBar(int cols);
  void drawOpponentPane(int y, int x, int h, int w);
  void drawDeskPane(int y, int x, int h, int w);
  void drawHandPane(int y, int x, int h, int w);
  void drawCapturedYakuPane(int y, int x, int h, int w);
  void drawStatusPromptPane(int y, int x, int h, int w);
  void drawGraphPane(int y, int x, int h, int w);
  void drawLogPane(int y, int x, int h, int w);
  void drawBottomKeyBar(int y, int cols);

  // Braille chart helper
  void renderBrailleChart(int y, int x, int h, int w, int item_idx,
                          bool show_axes);

  // Actions & Modals (1:1 parity with Qt6 MainWindow & Dialogs)
  void actionPlaySelectedCard();
  void actionNextRound();
  void actionDecisionYes();
  void actionDecisionNo();
  void actionNewGame();
  void runBotTurnsIfNeeded();

  void showSettingsDialog();
  void showYakuHelpDialog();
  void showDeckInfoDialog();
  void showHistoryDialog(int initial_item);
  void showAboutDialog();
  void showDocsDialog();
  void showHighscoresDialog();
  void showHelpDialog();

  // Generic UI Dialog primitives
  void showMessageModal(const std::string& title, const std::string& message,
                        int border_color = 1);
  bool showConfirmModal(const std::string& title, const std::string& message);

  void clampCursors();
  static std::string itemName(int item_idx);

  GameState _gameState;
  FocusPane _focus = FocusPane::Hand;
  int _handCursor = 0;
  int _deskChoiceCursor = 0;
  int _chartItemIdx = ITEM_TOTAL_SCORE;
  bool _running = true;
};
