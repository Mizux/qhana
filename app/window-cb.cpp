#include "window-cb.h"

#include <QDialog>
#include <QMessageBox>
#include <format>
#include <string>

#include "config.h"
#include "qhana.h"
#include "window.h"

void window_main_button_play_clicked_cb(MainWindow& window) {
  auto& gs = window.gameState();
  if (gs.phase == GamePhase::SelectHandCard && gs.current_player == 0) {
    int idx = window.boardWidget() ? window.boardWidget()->selectedHandIndex() : 0;
    if (idx < 0 || idx >= static_cast<int>(gs.players[0].hand.size())) {
      idx = 0;
    }
    window.onHandCardClicked(idx);
  }
}

void window_main_button_next_round_clicked_cb(MainWindow& window) {
  auto& gs = window.gameState();
  if (gs.phase == GamePhase::RoundOver) {
    gs.new_round();
    window.updateAllUi();
  }
}

void window_main_button_yes_clicked_cb(MainWindow& window) {
  auto& gs = window.gameState();
  if (gs.phase == GamePhase::AskKoiKoi) {
    gs.answer_koikoi(true);
    window.updateAllUi();
  } else if (gs.phase == GamePhase::AskDoubleUp) {
    gs.answer_double_up(true);
    window.updateAllUi();
  } else if (gs.phase == GamePhase::AskBigOrSmall) {
    gs.answer_big_or_small(true);
    window.updateAllUi();
  } else if (gs.phase == GamePhase::SelectDeskCardForHand ||
             gs.phase == GamePhase::SelectDeskCardForDrawn) {
    gs.select_desk_card(gs.desk_choice_slots[0]);
    window.updateAllUi();
  }
}

void window_main_button_no_clicked_cb(MainWindow& window) {
  auto& gs = window.gameState();
  if (gs.phase == GamePhase::AskKoiKoi) {
    gs.answer_koikoi(false);
    window.updateAllUi();
  } else if (gs.phase == GamePhase::AskDoubleUp) {
    gs.answer_double_up(false);
    window.updateAllUi();
  } else if (gs.phase == GamePhase::AskBigOrSmall) {
    gs.answer_big_or_small(false);
    window.updateAllUi();
  } else if (gs.phase == GamePhase::SelectDeskCardForHand ||
             gs.phase == GamePhase::SelectDeskCardForDrawn) {
    gs.select_desk_card(gs.desk_choice_slots[1]);
    window.updateAllUi();
  }
}

void window_main_button_settings_clicked_cb(MainWindow& window) {
  WindowSettings dlg(window.gameState(), &window);
  bool mode_changed = false;
  QObject::connect(&dlg, &WindowSettings::settingsApplied,
                   [&mode_changed](bool changed) { mode_changed = changed; });
  if (dlg.exec() == QDialog::Accepted) {
    if (mode_changed) {
      window.gameState().new_game();
    }
    window.updateAllUi();
  }
}

void window_main_button_yaku_help_clicked_cb(MainWindow& window) {
  WindowYakuHelp dlg(window.gameState(), window.boardWidget(), &window);
  dlg.exec();
}

void window_main_button_deck_info_clicked_cb(MainWindow& window) {
  WindowDeckInfo dlg(window.gameState(), window.boardWidget(), &window);
  dlg.exec();
}

void window_main_button_history_clicked_cb(MainWindow& window) {
  int item_idx = window.statusChartView()
                     ? window.statusChartView()->itemIndex()
                     : ScoreChartView::ITEM_TOTAL_SCORE;
  WindowHistory dlg(window.gameState(), item_idx, &window);
  dlg.exec();
}

void window_main_button_about_clicked_cb(MainWindow& window) {
  std::string info = std::format(
      "{}\n{}\n\nBased on SDLHana by Wei Mingzhi\nAuthor: {}\nVersion: {}",
      kProgramName, kProgramDescription, kProgramAuthorName, kProgramVersion);
  QMessageBox::about(&window, "About QHana", QString::fromStdString(info));
}

void window_main_button_docs_clicked_cb(MainWindow& window) {
  QMessageBox::information(
      &window, "Hanafuda Rules & Documentation",
      QString::fromUtf8(
          "Welcome to QHana (Hanafuda)!\n\n"
          "Hanafuda uses a deck of 48 flower cards divided into 12 months "
          "(4 cards per month).\n\n"
          "• Turn Structure:\n"
          "  1. Select a card from your hand. If it matches the month of a "
          "card on the table, you capture both! Otherwise it stays on the "
          "table.\n"
          "  2. A card is drawn from the draw pile and matched against the "
          "table in the same way.\n\n"
          "• Koi-Koi / Go-Stop:\n"
          "  When you complete a scoring combination (Yaku), you can choose "
          "to STOP and claim your points immediately, or call KOI-KOI / GO to "
          "continue playing for more points (at the risk of your opponent "
          "forming a Yaku first!).\n\n"
          "• Shortcuts:\n"
          "  [1-8] or Click: Play card from hand\n"
          "  [Left/Right]: Move hand selection\n"
          "  [Enter/Space]: Play selected card / Next round\n"
          "  [Y / N]: Yes (Koi-Koi / Big) or No (Stop / Small)"));
}

void window_main_button_highscores_clicked_cb(MainWindow& window) {
  const auto& gs = window.gameState();
  int best_score = gs.score;
  for (int s : gs.score_history) {
    if (s > best_score) best_score = s;
  }
  std::string text = std::format(
      "Session Statistics & High Scores:\n\n"
      "Current Score: {} pts\n"
      "Peak Session Score: {} pts\n"
      "Rounds Played: {}\n"
      "Wins: {}  |  Losses: {}  |  Draws: {}\n\n"
      "Hall of Fame:\n"
      "1. Hanafuda Master — 500 pts\n"
      "2. Koi-Koi Veteran — 200 pts\n"
      "3. Go-Stop Tactician — 100 pts\n"
      "4. Tea House Regular — 50 pts\n"
      "5. Novice — 10 pts",
      gs.score, best_score, gs.round_number, gs.rounds_won, gs.rounds_lost,
      gs.rounds_drawn);
  QMessageBox::information(&window, "High Scores",
                           QString::fromStdString(text));
}

void window_main_button_newgame_clicked_cb(MainWindow& window) {
  window.gameState().new_game();
  window.updateAllUi();
}
