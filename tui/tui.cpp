#include "tui.h"

#include <ncurses.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <sstream>
#include <string>
#include <vector>

#include "config.h"

namespace {

// Color Pair IDs (btop-inspired palette)
enum ColorPair : short {
  CP_DEFAULT = 1,
  CP_BORDER = 2,
  CP_BORDER_ACTIVE = 3,
  CP_TITLE = 4,
  CP_KEY = 5,
  CP_GREEN = 6,
  CP_YELLOW = 7,
  CP_RED = 8,
  CP_CYAN = 9,
  CP_MAGENTA = 10,
  CP_SELECTED = 11,
  CP_HEADER_BAR = 12,
  CP_DIM = 13,
  CP_GRAPH_LINE = 14,
  CP_GRAPH_REF = 15,
};

void init_btop_colors() {
  if (!has_colors()) return;
  start_color();
  use_default_colors();

  init_pair(CP_DEFAULT, COLOR_WHITE, -1);
  init_pair(CP_BORDER, COLOR_BLUE, -1);
  init_pair(CP_BORDER_ACTIVE, COLOR_CYAN, -1);
  init_pair(CP_TITLE, COLOR_WHITE, -1);
  init_pair(CP_KEY, COLOR_RED, -1);
  init_pair(CP_GREEN, COLOR_GREEN, -1);
  init_pair(CP_YELLOW, COLOR_YELLOW, -1);
  init_pair(CP_RED, COLOR_RED, -1);
  init_pair(CP_CYAN, COLOR_CYAN, -1);
  init_pair(CP_MAGENTA, COLOR_MAGENTA, -1);
  init_pair(CP_SELECTED, COLOR_BLACK, COLOR_CYAN);
  init_pair(CP_HEADER_BAR, COLOR_WHITE, COLOR_BLUE);
  init_pair(CP_DIM, COLOR_BLUE, -1);
  init_pair(CP_GRAPH_LINE, COLOR_YELLOW, -1);
  init_pair(CP_GRAPH_REF, COLOR_CYAN, -1);
}

short card_type_color(CardType t) {
  switch (t) {
    case CardType::Light:
      return CP_YELLOW;
    case CardType::Animal:
      return CP_MAGENTA;
    case CardType::RibbonBlue:
      return CP_CYAN;
    case CardType::RibbonRed:
      return CP_RED;
    case CardType::Ribbon:
      return CP_GREEN;
    case CardType::None:
    default:
      return CP_DEFAULT;
  }
}

// Draw a btop-style rounded box with embedded title and key hints
void draw_btop_box(int y, int x, int h, int w, const std::string& title,
                   const std::string& right_hint = "", bool active = false,
                   short custom_border_cp = 0) {
  if (h < 2 || w < 4) return;
  short b_cp = custom_border_cp
                   ? custom_border_cp
                   : (active ? CP_BORDER_ACTIVE : CP_BORDER);

  attron(COLOR_PAIR(b_cp) | (active ? A_BOLD : A_NORMAL));
  mvaddstr(y, x, "╭");
  for (int i = 1; i < w - 1; ++i) mvaddstr(y, x + i, "─");
  mvaddstr(y, x + w - 1, "╮");

  for (int r = 1; r < h - 1; ++r) {
    mvaddstr(y + r, x, "│");
    for (int c = 1; c < w - 1; ++c) mvaddch(y + r, x + c, ' ');
    mvaddstr(y + r, x + w - 1, "│");
  }

  mvaddstr(y + h - 1, x, "╰");
  for (int i = 1; i < w - 1; ++i) mvaddstr(y + h - 1, x + i, "─");
  mvaddstr(y + h - 1, x + w - 1, "╯");
  attroff(COLOR_PAIR(b_cp) | (active ? A_BOLD : A_NORMAL));

  if (!title.empty() && w > 8) {
    attron(COLOR_PAIR(b_cp) | (active ? A_BOLD : A_NORMAL));
    mvaddstr(y, x + 2, "┤ ");
    attroff(COLOR_PAIR(b_cp) | (active ? A_BOLD : A_NORMAL));

    attron(COLOR_PAIR(active ? CP_CYAN : CP_TITLE) | A_BOLD);
    std::string t = title;
    if (static_cast<int>(t.size()) > w - 8) t = t.substr(0, w - 8);
    addstr(t.c_str());
    attroff(COLOR_PAIR(active ? CP_CYAN : CP_TITLE) | A_BOLD);

    attron(COLOR_PAIR(b_cp) | (active ? A_BOLD : A_NORMAL));
    addstr(" ├");
    attroff(COLOR_PAIR(b_cp) | (active ? A_BOLD : A_NORMAL));
  }

  if (!right_hint.empty() &&
      static_cast<int>(right_hint.size() + title.size() + 12) < w) {
    int rx = x + w - static_cast<int>(right_hint.size()) - 6;
    attron(COLOR_PAIR(b_cp));
    mvaddstr(y, rx, "┤ ");
    attroff(COLOR_PAIR(b_cp));

    bool in_bracket = false;
    for (char ch : right_hint) {
      if (ch == '[') {
        in_bracket = true;
        attron(COLOR_PAIR(CP_DIM));
        addch('[');
        attroff(COLOR_PAIR(CP_DIM));
      } else if (ch == ']') {
        in_bracket = false;
        attron(COLOR_PAIR(CP_DIM));
        addch(']');
        attroff(COLOR_PAIR(CP_DIM));
      } else if (in_bracket) {
        attron(COLOR_PAIR(CP_KEY) | A_BOLD);
        addch(ch);
        attroff(COLOR_PAIR(CP_KEY) | A_BOLD);
      } else {
        attron(COLOR_PAIR(CP_TITLE));
        addch(ch);
        attroff(COLOR_PAIR(CP_TITLE));
      }
    }

    attron(COLOR_PAIR(b_cp));
    addstr(" ├");
    attroff(COLOR_PAIR(b_cp));
  }
}

void draw_progress_bar(int y, int x, int width, double ratio, short color_cp) {
  if (width <= 0) return;
  ratio = std::clamp(ratio, 0.0, 1.0);
  int filled = static_cast<int>(std::round(ratio * width));
  attron(COLOR_PAIR(color_cp) | A_BOLD);
  for (int i = 0; i < filled; ++i) {
    mvaddstr(y, x + i, "█");
  }
  attroff(COLOR_PAIR(color_cp) | A_BOLD);

  attron(COLOR_PAIR(CP_DIM));
  for (int i = filled; i < width; ++i) {
    mvaddstr(y, x + i, "░");
  }
  attroff(COLOR_PAIR(CP_DIM));
}

std::vector<std::string> wrap_text(const std::string& text, int max_width) {
  std::vector<std::string> lines;
  if (max_width <= 4) return lines;
  std::istringstream paragraphs(text);
  std::string para;
  while (std::getline(paragraphs, para, '\n')) {
    if (para.empty()) {
      lines.push_back("");
      continue;
    }
    std::istringstream words(para);
    std::string word;
    std::string current;
    while (words >> word) {
      if (current.empty()) {
        current = word;
      } else if (static_cast<int>(current.size() + 1 + word.size()) <=
                 max_width) {
        current += " " + word;
      } else {
        lines.push_back(current);
        current = word;
      }
    }
    if (!current.empty()) lines.push_back(current);
  }
  return lines;
}

std::string braille_utf8(uint8_t mask) {
  char buf[4];
  buf[0] = static_cast<char>(0xE2);
  buf[1] = static_cast<char>(0xA0 | ((mask >> 6) & 0x03));
  buf[2] = static_cast<char>(0x80 | (mask & 0x3F));
  buf[3] = '\0';
  return std::string(buf);
}

}  // namespace

TuiApp::TuiApp() : _gameState() { clampCursors(); }

TuiApp::~TuiApp() = default;

std::string TuiApp::itemName(int item_idx) {
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

void TuiApp::clampCursors() {
  int h_sz = static_cast<int>(_gameState.players[0].hand.size());
  if (h_sz <= 0) {
    _handCursor = 0;
  } else {
    _handCursor = std::clamp(_handCursor, 0, h_sz - 1);
  }
  _deskChoiceCursor = std::clamp(_deskChoiceCursor, 0, 1);
}

void TuiApp::runBotTurnsIfNeeded() {
  while (_gameState.phase == GamePhase::SelectHandCard &&
         _gameState.current_player == 1) {
    clampCursors();
    drawDashboard();
    napms(std::clamp(anim_speed_ms(_gameState.anim_speed), 50, 800));
    _gameState.step_bot();
    if (_gameState.sound_enabled) beep();
    clampCursors();
  }
}

int TuiApp::run() {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);
  set_escdelay(25);
  mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, nullptr);
  init_btop_colors();

  while (_running) {
    runBotTurnsIfNeeded();
    clampCursors();
    drawDashboard();

    int ch = getch();
    int hand_sz = static_cast<int>(_gameState.players[0].hand.size());

    switch (ch) {
      case 'q':
      case 'Q':
        if (showConfirmModal("Quit QHana",
                             "Are you sure you want to exit QHana TUI?")) {
          _running = false;
        }
        break;

      case '\t':
        _focus =
            (_focus == FocusPane::Hand) ? FocusPane::Desk : FocusPane::Hand;
        break;

      case KEY_UP:
      case 'k':
      case KEY_LEFT:
      case 'h':
        if (_gameState.phase == GamePhase::SelectDeskCardForHand ||
            _gameState.phase == GamePhase::SelectDeskCardForDrawn) {
          _deskChoiceCursor = 1 - _deskChoiceCursor;
        } else if (hand_sz > 0) {
          _handCursor = (_handCursor - 1 + hand_sz) % hand_sz;
        }
        break;

      case KEY_DOWN:
      case 'j':
      case KEY_RIGHT:
      case 'l':
        if (_gameState.phase == GamePhase::SelectDeskCardForHand ||
            _gameState.phase == GamePhase::SelectDeskCardForDrawn) {
          _deskChoiceCursor = 1 - _deskChoiceCursor;
        } else if (hand_sz > 0) {
          _handCursor = (_handCursor + 1) % hand_sz;
        }
        break;

      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8': {
        int idx = ch - '1';
        if (_gameState.phase == GamePhase::SelectDeskCardForHand ||
            _gameState.phase == GamePhase::SelectDeskCardForDrawn) {
          if (idx == 0 || idx == 1) {
            _deskChoiceCursor = idx;
            _gameState.select_desk_card(_gameState.desk_choice_slots[idx]);
            if (_gameState.sound_enabled) beep();
          }
        } else if (_gameState.phase == GamePhase::SelectHandCard &&
                   _gameState.current_player == 0 && idx < hand_sz) {
          _handCursor = idx;
          actionPlaySelectedCard();
        }
        break;
      }

      case '\n':
      case KEY_ENTER:
      case ' ':
        if (_gameState.phase == GamePhase::RoundOver) {
          actionNextRound();
        } else if (_gameState.phase == GamePhase::SelectDeskCardForHand ||
                   _gameState.phase == GamePhase::SelectDeskCardForDrawn) {
          _gameState.select_desk_card(
              _gameState.desk_choice_slots[_deskChoiceCursor]);
          if (_gameState.sound_enabled) beep();
        } else if (_gameState.phase == GamePhase::AskKoiKoi ||
                   _gameState.phase == GamePhase::AskDoubleUp ||
                   _gameState.phase == GamePhase::AskBigOrSmall) {
          actionDecisionYes();
        } else {
          actionPlaySelectedCard();
        }
        break;

      case 'y':
      case 'Y':
      case 'b':
      case 'B':
        actionDecisionYes();
        break;

      case 'n':
        if (_gameState.phase == GamePhase::AskKoiKoi ||
            _gameState.phase == GamePhase::AskDoubleUp ||
            _gameState.phase == GamePhase::AskBigOrSmall ||
            _gameState.phase == GamePhase::SelectDeskCardForHand ||
            _gameState.phase == GamePhase::SelectDeskCardForDrawn) {
          actionDecisionNo();
        } else {
          actionNewGame();
        }
        break;

      case 'N':
        actionNewGame();
        break;

      case 'r':
      case 'R':
        actionNextRound();
        break;

      case 's':
        if (_gameState.phase == GamePhase::AskBigOrSmall) {
          actionDecisionNo();
        } else {
          showSettingsDialog();
        }
        break;

      case 'S':
        showSettingsDialog();
        break;

      case 'v':
      case 'V':
        showYakuHelpDialog();
        break;

      case 'd':
      case 'D':
      case 'i':
      case 'I':
        showDeckInfoDialog();
        break;

      case 'g':
      case 'G':
      case 'z':
      case 'Z':
        showHistoryDialog(_chartItemIdx);
        break;

      case 'c':
      case 'C':
        _chartItemIdx = (_chartItemIdx + 1) % TOTAL_ITEMS;
        break;

      case 'm':
      case 'M':
        _gameState.sound_enabled = !_gameState.sound_enabled;
        if (_gameState.sound_enabled) beep();
        break;

      case 'a':
      case 'A':
        showAboutDialog();
        break;

      case 'o':
      case 'O':
        showDocsDialog();
        break;

      case 'H':
        showHighscoresDialog();
        break;

      case '?':
      case KEY_F(1):
        showHelpDialog();
        break;

      default:
        break;
    }
  }

  endwin();
  return 0;
}

void TuiApp::drawDashboard() {
  erase();
  int rows = 0, cols = 0;
  getmaxyx(stdscr, rows, cols);

  if (rows < 24 || cols < 78) {
    draw_btop_box(0, 0, rows, cols, "QHana TUI", "", true, CP_RED);
    attron(COLOR_PAIR(CP_YELLOW) | A_BOLD);
    mvprintw(rows / 2 - 1, std::max(2, (cols - 46) / 2),
             "Terminal window too small (%dx%d)!", cols, rows);
    mvprintw(rows / 2 + 1, std::max(2, (cols - 46) / 2),
             "Please resize to at least 78x24 for btop layout.");
    attroff(COLOR_PAIR(CP_YELLOW) | A_BOLD);
    refresh();
    return;
  }

  drawTopBar(cols);

  int log_h = std::clamp(rows / 5, 5, 7);
  int main_y = 1;
  int main_h = rows - 2 - log_h;
  int left_w = std::max(44, (cols * 52) / 100);
  int right_w = cols - left_w;

  // Left column: Opponent (5 rows), Desk/Table (middle), Your Hand (bottom)
  int opp_h = 5;
  int hand_h = std::max(10, (main_h * 42) / 100);
  int desk_h = main_h - opp_h - hand_h;
  if (desk_h < 7) {
    hand_h = std::max(8, main_h - opp_h - 7);
    desk_h = main_h - opp_h - hand_h;
  }

  // Right column: Captured & Yaku, Status & Prompt, Score Graph
  int yaku_h = std::max(9, (main_h * 42) / 100);
  int status_h = 6;
  int graph_h = main_h - yaku_h - status_h;
  if (graph_h < 5) {
    yaku_h = std::max(7, main_h - status_h - 5);
    graph_h = main_h - yaku_h - status_h;
  }

  drawOpponentPane(main_y, 0, opp_h, left_w);
  drawDeskPane(main_y + opp_h, 0, desk_h, left_w);
  drawHandPane(main_y + opp_h + desk_h, 0, hand_h, left_w);

  drawCapturedYakuPane(main_y, left_w, yaku_h, right_w);
  drawStatusPromptPane(main_y + yaku_h, left_w, status_h, right_w);
  drawGraphPane(main_y + yaku_h + status_h, left_w, graph_h, right_w);

  drawLogPane(main_y + main_h, 0, log_h, cols);
  drawBottomKeyBar(rows - 1, cols);

  refresh();
}

void TuiApp::drawTopBar(int cols) {
  attron(COLOR_PAIR(CP_HEADER_BAR) | A_BOLD);
  for (int c = 0; c < cols; ++c) mvaddch(0, c, ' ');

  std::string mode_str =
      game_mode_name(_gameState.mode, _gameState.language);
  std::string dealer_str = (_gameState.dealer == 0) ? "You" : "Computer";

  mvprintw(0, 1, "🎴 QHANA v%s │ 🎋 %s │ 🔄 Round #%d (Deck:%d) │ 👑 Dealer: %s",
           std::string(kProgramVersion).c_str(), mode_str.c_str(),
           _gameState.round_number, _gameState.remaining_deck_count(),
           dealer_str.c_str());

  std::string right_info = std::format(
      "Score: {} ({}W-{}L-{}D) │ Sound:{} │ [?]Help [q]Quit ",
      _gameState.score, _gameState.rounds_won, _gameState.rounds_lost,
      _gameState.rounds_drawn, _gameState.sound_enabled ? "ON" : "OFF");
  if (static_cast<int>(right_info.size()) + 48 < cols) {
    mvaddstr(0, cols - static_cast<int>(right_info.size()), right_info.c_str());
  }
  attroff(COLOR_PAIR(CP_HEADER_BAR) | A_BOLD);
}

void TuiApp::drawOpponentPane(int y, int x, int h, int w) {
  const auto& bot = _gameState.players[1];
  std::string right_hint =
      std::format("Round Pts: {}", bot.result.score);
  draw_btop_box(y, x, h, w, "Computer Opponent", right_hint, false);

  // Row 1: Hand cards
  mvprintw(y + 1, x + 2, "Hand (%zu): ", bot.hand.size());
  attron(COLOR_PAIR(CP_RED) | A_BOLD);
  for (size_t i = 0; i < bot.hand.size(); ++i) {
    if (_gameState.phase == GamePhase::RoundOver) {
      addstr(std::format("[{:02d}{}] ", bot.hand[i].month(),
                         card_short_label(bot.hand[i]))
                 .c_str());
    } else {
      addstr("[🎴] ");
    }
  }
  attroff(COLOR_PAIR(CP_RED) | A_BOLD);

  // Row 2: Captured summary
  mvprintw(y + 2, x + 2, "Captured: ");
  attron(COLOR_PAIR(CP_YELLOW) | A_BOLD);
  printw("★Lgt:%d ", bot.count_lights());
  attroff(COLOR_PAIR(CP_YELLOW) | A_BOLD);
  attron(COLOR_PAIR(CP_MAGENTA) | A_BOLD);
  printw("◆Ani:%d ", bot.count_animals());
  attroff(COLOR_PAIR(CP_MAGENTA) | A_BOLD);
  attron(COLOR_PAIR(CP_CYAN) | A_BOLD);
  printw("▬Rib:%d ", bot.count_ribbons());
  attroff(COLOR_PAIR(CP_CYAN) | A_BOLD);
  attron(COLOR_PAIR(CP_DEFAULT));
  printw("·Chf:%d", bot.count_chaff(_gameState.mode));
  attroff(COLOR_PAIR(CP_DEFAULT));

  // Row 3: Computer captured specials
  if (h >= 5) {
    auto spec = bot.captured_special();
    mvprintw(y + 3, x + 2, "Specials: ");
    int cur_x = x + 12;
    for (const auto& c : spec) {
      std::string tok =
          std::format("{}M:{} ", c.month(), card_short_label(c));
      if (cur_x + static_cast<int>(tok.size()) >= x + w - 2) break;
      short cp = card_type_color(c.type());
      attron(COLOR_PAIR(cp) | A_BOLD);
      mvaddstr(y + 3, cur_x, tok.c_str());
      attroff(COLOR_PAIR(cp) | A_BOLD);
      cur_x += static_cast<int>(tok.size());
    }
    if (spec.empty()) {
      attron(COLOR_PAIR(CP_DIM));
      mvaddstr(y + 3, cur_x, "(none)");
      attroff(COLOR_PAIR(CP_DIM));
    }
  }
}

void TuiApp::drawDeskPane(int y, int x, int h, int w) {
  bool choosing_desk =
      (_gameState.phase == GamePhase::SelectDeskCardForHand ||
       _gameState.phase == GamePhase::SelectDeskCardForDrawn);
  bool active = choosing_desk || (_focus == FocusPane::Desk);

  std::string hint = choosing_desk
                         ? "[←/→/1/2]Choose Match [Enter]Capture"
                         : std::format("Draw Pile: {} cards",
                                       _gameState.remaining_deck_count());
  draw_btop_box(y, x, h, w, "¹Hanafuda Table (Desk)", hint, active,
                choosing_desk ? CP_YELLOW : 0);

  int selected_hand_month = -1;
  if (_gameState.phase == GamePhase::SelectHandCard &&
      _gameState.current_player == 0 &&
      _handCursor >= 0 &&
      _handCursor < static_cast<int>(_gameState.players[0].hand.size())) {
    selected_hand_month = _gameState.players[0].hand[_handCursor].month();
  }

  // Collect valid desk slots
  std::vector<int> slots;
  for (int i = 0; i < _gameState.num_desk_cards; ++i) {
    if (_gameState.desk_cards[i].is_valid()) {
      slots.push_back(i);
    }
  }

  if (slots.empty()) {
    attron(COLOR_PAIR(CP_DIM));
    mvaddstr(y + h / 2, x + 4, "(Table is currently empty)");
    attroff(COLOR_PAIR(CP_DIM));
    return;
  }

  int col_w = (w - 4) / 2;
  int max_rows = h - 2;

  for (size_t idx = 0; idx < slots.size(); ++idx) {
    int slot = slots[idx];
    const Card& c = _gameState.desk_cards[slot];
    int r = static_cast<int>(idx) / 2;
    int col = static_cast<int>(idx) % 2;
    if (r >= max_rows) break;

    int ry = y + 1 + r;
    int rx = x + 2 + col * col_w;

    bool is_choice0 = choosing_desk && (slot == _gameState.desk_choice_slots[0]);
    bool is_choice1 = choosing_desk && (slot == _gameState.desk_choice_slots[1]);
    bool is_chosen_cursor =
        (is_choice0 && _deskChoiceCursor == 0) ||
        (is_choice1 && _deskChoiceCursor == 1);
    bool matches_hand =
        (!choosing_desk && selected_hand_month == c.month());

    short cp = card_type_color(c.type());
    if (is_chosen_cursor) {
      attron(COLOR_PAIR(CP_SELECTED) | A_BOLD);
    } else if (is_choice0 || is_choice1) {
      attron(COLOR_PAIR(CP_YELLOW) | A_BOLD);
    } else {
      attron(COLOR_PAIR(cp) | (matches_hand ? A_BOLD : A_NORMAL));
    }

    std::string prefix = "  ";
    if (is_choice0)
      prefix = "[1]";
    else if (is_choice1)
      prefix = "[2]";
    else if (matches_hand)
      prefix = "▶ ";

    std::string badge = card_type_badge(c.type());
    std::string line =
        std::format("{:<3} {:2d}M {:<6} {}", prefix, c.month(),
                    card_short_label(c), badge);
    if (static_cast<int>(line.size()) > col_w - 1) {
      line = line.substr(0, col_w - 1);
    }
    mvprintw(ry, rx, "%-*s", col_w - 1, line.c_str());

    if (is_chosen_cursor) {
      attroff(COLOR_PAIR(CP_SELECTED) | A_BOLD);
    } else if (is_choice0 || is_choice1) {
      attroff(COLOR_PAIR(CP_YELLOW) | A_BOLD);
    } else {
      attroff(COLOR_PAIR(cp) | (matches_hand ? A_BOLD : A_NORMAL));
    }
  }
}

void TuiApp::drawHandPane(int y, int x, int h, int w) {
  bool active = (_focus == FocusPane::Hand) &&
                (_gameState.phase == GamePhase::SelectHandCard);
  draw_btop_box(y, x, h, w, "²Your Hand", "[↑/↓/1-8]Select [Enter]Play",
                active);

  attron(COLOR_PAIR(CP_CYAN) | A_BOLD);
  mvprintw(y + 1, x + 2, "%-3s %-6s %-26s %-10s", "#", "Type", "Card Name",
           "Table Match");
  attroff(COLOR_PAIR(CP_CYAN) | A_BOLD);

  const auto& hand = _gameState.players[0].hand;
  int max_rows = h - 3;

  for (int i = 0; i < static_cast<int>(hand.size()) && i < max_rows; ++i) {
    const Card& c = hand[i];
    bool selected = (i == _handCursor);
    auto matches = _gameState.matching_desk_slots(c.month());

    int ry = y + 2 + i;
    if (selected) {
      attron(COLOR_PAIR(active ? CP_SELECTED : CP_CYAN) | A_BOLD);
      for (int col = 1; col < w - 1; ++col) mvaddch(ry, x + col, ' ');
    }

    short cp = card_type_color(c.type());
    if (!selected) attron(COLOR_PAIR(cp) | A_BOLD);
    mvprintw(ry, x + 2, "[%d] %-5s", i + 1, card_type_badge(c.type()).c_str());
    if (!selected) attroff(COLOR_PAIR(cp) | A_BOLD);

    std::string cname = card_name(c);
    if (cname.size() > 26) cname = cname.substr(0, 26);
    if (!selected) attron(COLOR_PAIR(CP_DEFAULT));
    mvprintw(ry, x + 12, "%-26s", cname.c_str());
    if (!selected) attroff(COLOR_PAIR(CP_DEFAULT));

    std::string m_str =
        matches.empty()
            ? "· None"
            : std::format("✓ {} match{}", matches.size(),
                          matches.size() > 1 ? "es" : "");
    if (!selected) {
      attron(COLOR_PAIR(matches.empty() ? CP_DIM : CP_GREEN) |
             (matches.empty() ? A_NORMAL : A_BOLD));
    }
    if (x + 39 < x + w - 2) {
      mvprintw(ry, x + 39, "%s", m_str.c_str());
    }
    if (!selected) {
      attroff(COLOR_PAIR(matches.empty() ? CP_DIM : CP_GREEN) |
              (matches.empty() ? A_NORMAL : A_BOLD));
    }

    if (selected) {
      attroff(COLOR_PAIR(active ? CP_SELECTED : CP_CYAN) | A_BOLD);
    }
  }

  if (hand.empty()) {
    attron(COLOR_PAIR(CP_DIM));
    mvaddstr(y + 3, x + 4, "(No cards remaining in hand)");
    attroff(COLOR_PAIR(CP_DIM));
  }
}

void TuiApp::drawCapturedYakuPane(int y, int x, int h, int w) {
  const auto& p0 = _gameState.players[0];
  std::string hint = std::format("[v]YakuRef [d]Deck (You: {} pts)",
                                 p0.result.score);
  draw_btop_box(y, x, h, w, "Your Captured Cards & Yaku", hint, false);

  int bar_w = std::clamp(w - 28, 6, 16);

  // Lights
  int lgt = p0.count_lights();
  mvprintw(y + 1, x + 2, "★ Lights  (%d/5):", lgt);
  draw_progress_bar(y + 1, x + 19, bar_w, lgt / 5.0, CP_YELLOW);

  // Animals
  int ani = p0.count_animals();
  mvprintw(y + 2, x + 2, "◆ Animals (%d/5):", ani);
  draw_progress_bar(y + 2, x + 19, bar_w, ani / 5.0, CP_MAGENTA);

  // Ribbons
  int rib = p0.count_ribbons();
  mvprintw(y + 3, x + 2, "▬ Ribbons (%d/5):", rib);
  draw_progress_bar(y + 3, x + 19, bar_w, rib / 5.0, CP_CYAN);

  // Chaff
  int chf = p0.count_chaff(_gameState.mode);
  mvprintw(y + 4, x + 2, "· Chaff  (%2d/10):", chf);
  draw_progress_bar(y + 4, x + 19, bar_w, chf / 10.0, CP_GREEN);

  // Captured special list
  auto spec = p0.captured_special();
  mvprintw(y + 5, x + 2, "Specials: ");
  int cur_x = x + 12;
  for (const auto& c : spec) {
    std::string tok = std::format("{}M:{} ", c.month(), card_short_label(c));
    if (cur_x + static_cast<int>(tok.size()) >= x + w - 2) break;
    short cp = card_type_color(c.type());
    attron(COLOR_PAIR(cp) | A_BOLD);
    mvaddstr(y + 5, cur_x, tok.c_str());
    attroff(COLOR_PAIR(cp) | A_BOLD);
    cur_x += static_cast<int>(tok.size());
  }
  if (spec.empty()) {
    attron(COLOR_PAIR(CP_DIM));
    mvaddstr(y + 5, cur_x, "(none yet)");
    attroff(COLOR_PAIR(CP_DIM));
  }

  // Active Yaku list
  auto y0 = _gameState.get_yaku_items(0, false);
  auto y1 = _gameState.get_yaku_items(1, false);
  int row = y + 6;
  for (const auto& yk : y0) {
    if (row >= y + h - 1) break;
    attron(COLOR_PAIR(CP_GREEN) | A_BOLD);
    mvprintw(row++, x + 2, "✓ You: %-20s +%d pts", yk.name.c_str(), yk.points);
    attroff(COLOR_PAIR(CP_GREEN) | A_BOLD);
  }
  for (const auto& yk : y1) {
    if (row >= y + h - 1) break;
    attron(COLOR_PAIR(CP_RED) | A_BOLD);
    mvprintw(row++, x + 2, "⚠ Com: %-20s +%d pts", yk.name.c_str(), yk.points);
    attroff(COLOR_PAIR(CP_RED) | A_BOLD);
  }
  if (y0.empty() && y1.empty() && row < y + h - 1) {
    attron(COLOR_PAIR(CP_DIM));
    mvaddstr(row, x + 2, "No Yaku completed yet this round.");
    attroff(COLOR_PAIR(CP_DIM));
  }
}

void TuiApp::drawStatusPromptPane(int y, int x, int h, int w) {
  bool is_prompt = (_gameState.phase == GamePhase::AskKoiKoi ||
                    _gameState.phase == GamePhase::AskDoubleUp ||
                    _gameState.phase == GamePhase::AskBigOrSmall ||
                    _gameState.phase == GamePhase::SelectDeskCardForHand ||
                    _gameState.phase == GamePhase::SelectDeskCardForDrawn);
  short border_cp = is_prompt
                        ? CP_YELLOW
                        : (_gameState.phase == GamePhase::RoundOver ? CP_GREEN : 0);

  draw_btop_box(y, x, h, w, "Turn Status & Action Prompt", "[s]Settings [n]New",
                is_prompt, border_cp);

  attron(COLOR_PAIR(CP_YELLOW) | A_BOLD);
  std::string banner = _gameState.status_banner;
  if (static_cast<int>(banner.size()) > w - 4) {
    banner = banner.substr(0, w - 4);
  }
  mvaddstr(y + 1, x + 2, banner.c_str());
  attroff(COLOR_PAIR(CP_YELLOW) | A_BOLD);

  if (_gameState.phase == GamePhase::AskKoiKoi) {
    attron(COLOR_PAIR(CP_GREEN) | A_BOLD);
    mvaddstr(y + 3, x + 2, "▶ Press [y] / [Enter] for YES (Koi-Koi / Go)");
    attroff(COLOR_PAIR(CP_GREEN) | A_BOLD);
    attron(COLOR_PAIR(CP_RED) | A_BOLD);
    mvaddstr(y + 4, x + 2, "▶ Press [n] for NO (Stop & Claim Points)");
    attroff(COLOR_PAIR(CP_RED) | A_BOLD);
  } else if (_gameState.phase == GamePhase::AskDoubleUp) {
    attron(COLOR_PAIR(CP_GREEN) | A_BOLD);
    mvaddstr(y + 3, x + 2, "▶ Press [y] to Double-Up | [n] to Keep Score");
    attroff(COLOR_PAIR(CP_GREEN) | A_BOLD);
  } else if (_gameState.phase == GamePhase::AskBigOrSmall) {
    attron(COLOR_PAIR(CP_CYAN) | A_BOLD);
    mvaddstr(y + 3, x + 2, "▶ Press [b]/[y] for BIG (Months 7-12)");
    mvaddstr(y + 4, x + 2, "▶ Press [s]/[n] for SMALL (Months 1-6)");
    attroff(COLOR_PAIR(CP_CYAN) | A_BOLD);
  } else if (_gameState.phase == GamePhase::SelectDeskCardForHand ||
             _gameState.phase == GamePhase::SelectDeskCardForDrawn) {
    attron(COLOR_PAIR(CP_CYAN) | A_BOLD);
    mvaddstr(y + 3, x + 2,
             "▶ Two table cards match! Press [1] or [2] (or ←/→ + Enter)");
    attroff(COLOR_PAIR(CP_CYAN) | A_BOLD);
  } else if (_gameState.phase == GamePhase::RoundOver) {
    attron(COLOR_PAIR(CP_GREEN) | A_BOLD);
    mvaddstr(y + 3, x + 2,
             "▶ Round finished! Press [Space], [Enter], or [r] for Next Round.");
    attroff(COLOR_PAIR(CP_GREEN) | A_BOLD);
  } else {
    mvprintw(y + 3, x + 2,
             "Last Played: %-18s  Drawn: %s",
             _gameState.last_played_card.is_valid()
                 ? card_short_label(_gameState.last_played_card).c_str()
                 : "-",
             _gameState.last_drawn_card.is_valid()
                 ? card_short_label(_gameState.last_drawn_card).c_str()
                 : "-");
  }
}

void TuiApp::drawGraphPane(int y, int x, int h, int w) {
  std::string title = std::format("Graph: {}", itemName(_chartItemIdx));
  draw_btop_box(y, x, h, w, title, "[c]Cycle [g]Zoom", false);
  if (h >= 5 && w >= 12) {
    renderBrailleChart(y + 1, x + 2, h - 2, w - 4, _chartItemIdx, false);
  }
}

void TuiApp::renderBrailleChart(int y, int x, int h, int w, int item_idx,
                                bool show_axes) {
  if (h <= 0 || w <= 0) return;

  int plot_x = x;
  int plot_w = w;
  if (show_axes && w > 14) {
    plot_x = x + 8;
    plot_w = w - 8;
  }

  const std::vector<int>* vec = &_gameState.score_history;
  int cur_v = _gameState.score;
  if (item_idx == ITEM_PLAYER_ROUND) {
    vec = &_gameState.player_round_scores;
    cur_v = _gameState.players[0].result.score;
  } else if (item_idx == ITEM_BOT_ROUND) {
    vec = &_gameState.bot_round_scores;
    cur_v = _gameState.players[1].result.score;
  }

  std::vector<double> samples;
  for (int v : *vec) samples.push_back(static_cast<double>(v));
  if (samples.empty() || _gameState.phase != GamePhase::RoundOver) {
    samples.push_back(static_cast<double>(cur_v));
  }

  double min_v = 0.0;
  double max_v = 10.0;
  for (double v : samples) {
    if (v < min_v) min_v = v;
    if (v > max_v) max_v = v;
  }
  if (max_v - min_v < 4.0) max_v = min_v + 4.0;

  if (show_axes && w > 14) {
    attron(COLOR_PAIR(CP_DIM));
    mvprintw(y, x, "%7.0f", max_v);
    mvprintw(y + h / 2, x, "%7.0f", (min_v + max_v) / 2.0);
    mvprintw(y + h - 1, x, "%7.0f", min_v);
    attroff(COLOR_PAIR(CP_DIM));
  }

  int px_w = plot_w * 2;
  int px_h = h * 4;
  std::vector<std::vector<uint8_t>> mask(h, std::vector<uint8_t>(plot_w, 0));

  auto set_dot = [&](int gx, int gy) {
    if (gx < 0 || gx >= px_w || gy < 0 || gy >= px_h) return;
    int cell_x = gx / 2;
    int cell_y = (px_h - 1 - gy) / 4;
    int sub_x = gx % 2;
    int sub_y = (px_h - 1 - gy) % 4;
    static constexpr uint8_t bit_map[4][2] = {
        {0x01, 0x08},
        {0x02, 0x10},
        {0x04, 0x20},
        {0x40, 0x80},
    };
    mask[cell_y][cell_x] |= bit_map[sub_y][sub_x];
  };

  int n = static_cast<int>(samples.size());
  for (int i = 0; i < n; ++i) {
    int gx0 = (n == 1) ? 0 : (i * (px_w - 1)) / std::max(1, n - 1);
    int gy0 = static_cast<int>(
        std::round(((samples[i] - min_v) / (max_v - min_v)) * (px_h - 1)));
    set_dot(gx0, gy0);

    if (i + 1 < n) {
      int gx1 = ((i + 1) * (px_w - 1)) / std::max(1, n - 1);
      int gy1 = static_cast<int>(std::round(
          ((samples[i + 1] - min_v) / (max_v - min_v)) * (px_h - 1)));
      int steps = std::max(std::abs(gx1 - gx0), std::abs(gy1 - gy0));
      for (int s = 1; s <= steps; ++s) {
        int lx = gx0 + ((gx1 - gx0) * s) / steps;
        int ly = gy0 + ((gy1 - gy0) * s) / steps;
        set_dot(lx, ly);
      }
    }
  }

  attron(COLOR_PAIR(CP_GRAPH_LINE) | A_BOLD);
  for (int r = 0; r < h; ++r) {
    for (int c = 0; c < plot_w; ++c) {
      if (mask[r][c] != 0) {
        mvaddstr(y + r, plot_x + c, braille_utf8(mask[r][c]).c_str());
      }
    }
  }
  attroff(COLOR_PAIR(CP_GRAPH_LINE) | A_BOLD);
}

void TuiApp::drawLogPane(int y, int x, int h, int w) {
  draw_btop_box(y, x, h, w, "Turn Log & Events", "[o]Docs [H]Scores [a]About",
                false);
  int max_lines = h - 2;
  int total = static_cast<int>(_gameState.game_log.size());
  int start = std::max(0, total - max_lines);

  for (int i = 0; i < max_lines && (start + i) < total; ++i) {
    std::string line = _gameState.game_log[start + i];
    if (static_cast<int>(line.size()) > w - 4) {
      line = line.substr(0, w - 4);
    }
    bool is_latest = (start + i == total - 1);
    if (is_latest) attron(COLOR_PAIR(CP_CYAN) | A_BOLD);
    mvaddstr(y + 1 + i, x + 2, line.c_str());
    if (is_latest) attroff(COLOR_PAIR(CP_CYAN) | A_BOLD);
  }
}

void TuiApp::drawBottomKeyBar(int y, int cols) {
  attron(COLOR_PAIR(CP_HEADER_BAR));
  for (int c = 0; c < cols; ++c) mvaddch(y, c, ' ');
  std::string bar =
      " [1-8/Enter]Play [y/n]Yes/No [r]NextRound [s]Settings [v]Yaku [d]Deck "
      "[g]Graph [n]New [?]Help [q]Quit";
  if (static_cast<int>(bar.size()) > cols) bar = bar.substr(0, cols);
  mvaddstr(y, 0, bar.c_str());
  attroff(COLOR_PAIR(CP_HEADER_BAR));
}

// ============================================================================
// Actions & Modals
// ============================================================================

void TuiApp::actionPlaySelectedCard() {
  if (_gameState.phase == GamePhase::SelectHandCard &&
      _gameState.current_player == 0) {
    if (_gameState.play_hand_card(_handCursor)) {
      if (_gameState.sound_enabled) beep();
      clampCursors();
    }
  }
}

void TuiApp::actionNextRound() {
  if (_gameState.phase == GamePhase::RoundOver) {
    _gameState.new_round();
    clampCursors();
  }
}

void TuiApp::actionDecisionYes() {
  if (_gameState.phase == GamePhase::AskKoiKoi) {
    _gameState.answer_koikoi(true);
    if (_gameState.sound_enabled) beep();
  } else if (_gameState.phase == GamePhase::AskDoubleUp) {
    _gameState.answer_double_up(true);
    if (_gameState.sound_enabled) beep();
  } else if (_gameState.phase == GamePhase::AskBigOrSmall) {
    _gameState.answer_big_or_small(true);
    if (_gameState.sound_enabled) beep();
  } else if (_gameState.phase == GamePhase::SelectDeskCardForHand ||
             _gameState.phase == GamePhase::SelectDeskCardForDrawn) {
    _gameState.select_desk_card(_gameState.desk_choice_slots[0]);
    if (_gameState.sound_enabled) beep();
  }
}

void TuiApp::actionDecisionNo() {
  if (_gameState.phase == GamePhase::AskKoiKoi) {
    _gameState.answer_koikoi(false);
    if (_gameState.sound_enabled) beep();
  } else if (_gameState.phase == GamePhase::AskDoubleUp) {
    _gameState.answer_double_up(false);
    if (_gameState.sound_enabled) beep();
  } else if (_gameState.phase == GamePhase::AskBigOrSmall) {
    _gameState.answer_big_or_small(false);
    if (_gameState.sound_enabled) beep();
  } else if (_gameState.phase == GamePhase::SelectDeskCardForHand ||
             _gameState.phase == GamePhase::SelectDeskCardForDrawn) {
    _gameState.select_desk_card(_gameState.desk_choice_slots[1]);
    if (_gameState.sound_enabled) beep();
  }
}

void TuiApp::actionNewGame() {
  if (showConfirmModal("New Game",
                       "Start a new Hanafuda game and reset current score?")) {
    _gameState.new_game();
    clampCursors();
  }
}

void TuiApp::showSettingsDialog() {
  int cursor = 0;
  GameMode old_mode = _gameState.mode;

  while (true) {
    drawDashboard();
    int rows = 0, cols = 0;
    getmaxyx(stdscr, rows, cols);
    int mh = 14, mw = 54;
    int my = (rows - mh) / 2, mx = (cols - mw) / 2;

    draw_btop_box(my, mx, mh, mw, "Game Settings", "[←/→]Change [Enter/Esc]Done",
                  true, CP_CYAN);

    std::array<std::string, 4> labels = {
        "Game Mode", "Language", "Animation Speed", "Sound Effects"};
    std::array<std::string, 4> values = {
        game_mode_name(_gameState.mode, _gameState.language),
        language_name(_gameState.language),
        anim_speed_name(_gameState.anim_speed),
        _gameState.sound_enabled ? "Enabled" : "Disabled"};

    for (int i = 0; i < 4; ++i) {
      int ry = my + 3 + i * 2;
      bool sel = (i == cursor);
      if (sel) attron(COLOR_PAIR(CP_SELECTED) | A_BOLD);
      mvprintw(ry, mx + 3, " %-18s :  ◀ %-20s ▶ ", labels[i].c_str(),
               values[i].c_str());
      if (sel) attroff(COLOR_PAIR(CP_SELECTED) | A_BOLD);
    }

    attron(COLOR_PAIR(CP_DIM));
    mvaddstr(my + mh - 2, mx + 4,
             "Note: Changing Game Mode starts a fresh game.");
    attroff(COLOR_PAIR(CP_DIM));
    refresh();

    int ch = getch();
    if (ch == 27 || ch == 'q' || ch == '\n' || ch == KEY_ENTER) {
      break;
    } else if (ch == KEY_UP || ch == 'k') {
      cursor = (cursor + 3) % 4;
    } else if (ch == KEY_DOWN || ch == 'j') {
      cursor = (cursor + 1) % 4;
    } else if (ch == KEY_LEFT || ch == 'h' || ch == KEY_RIGHT || ch == 'l' ||
               ch == ' ') {
      int dir = (ch == KEY_LEFT || ch == 'h') ? -1 : 1;
      if (cursor == 0) {
        int m = (static_cast<int>(_gameState.mode) + dir + 3) % 3;
        _gameState.mode = static_cast<GameMode>(m);
      } else if (cursor == 1) {
        int l = (static_cast<int>(_gameState.language) + dir + 3) % 3;
        _gameState.language = static_cast<Language>(l);
      } else if (cursor == 2) {
        int s = (static_cast<int>(_gameState.anim_speed) + dir + 5) % 5;
        _gameState.anim_speed = static_cast<AnimSpeed>(s);
      } else if (cursor == 3) {
        _gameState.sound_enabled = !_gameState.sound_enabled;
      }
    }
  }

  if (_gameState.mode != old_mode) {
    _gameState.new_game();
    clampCursors();
  }
}

void TuiApp::showYakuHelpDialog() {
  drawDashboard();
  int rows = 0, cols = 0;
  getmaxyx(stdscr, rows, cols);
  int mh = std::min(rows - 2, 20);
  int mw = std::min(cols - 4, 74);
  int my = (rows - mh) / 2, mx = (cols - mw) / 2;

  draw_btop_box(my, mx, mh, mw, "Hanafuda Yaku & Scoring Reference",
                "[Any Key] Close", true, CP_CYAN);

  attron(COLOR_PAIR(CP_YELLOW) | A_BOLD);
  mvprintw(my + 1, mx + 2, "%-24s %-9s %-9s %-26s", "Yaku Combination",
           "Koi-Koi", "Go-Stop", "Required Cards");
  attroff(COLOR_PAIR(CP_YELLOW) | A_BOLD);

  struct RefRow {
    const char* name;
    const char* jpn;
    const char* kor;
    const char* desc;
  };
  constexpr std::array<RefRow, 13> kRows = {{
      {"Five Lights (Goko)", "15 pts", "15 pts", "All 5 Light cards"},
      {"Four Lights (Shiko)", "10 pts", "4 pts", "4 Lights (no Rain Man)"},
      {"Rain Four Lights", "8 pts", "4 pts", "4 Lights (with Rain Man)"},
      {"Three Lights (Sanko)", "6 pts", "3 pts", "3 Lights (no Rain Man)"},
      {"Rain Three Lights", "-", "2 pts", "3 Lights (with Rain Man)"},
      {"Boar, Deer & Butterfly", "5 pts", "-", "Jun + Jul + Oct Animals"},
      {"Five Birds (Godori)", "-", "5 pts", "Feb + Apr + Aug Birds"},
      {"Flower Meets Sake Cup", "3 pts", "-", "Mar Curtain + Sep Sake Cup"},
      {"Moon Meets Sake Cup", "3 pts", "-", "Aug Moon + Sep Sake Cup"},
      {"Red Poetry Ribbons", "6 pts", "3 pts", "Jan + Feb + Mar Ribbons"},
      {"Blue Ribbons", "6 pts", "3 pts", "Jun + Sep + Oct Ribbons"},
      {"Grass Ribbons", "-", "3 pts", "Apr + May + Jul Ribbons"},
      {"Animals/Ribbons(5+)/Chaff", "1+ pts", "1+ pts", "5+ Ani/Rib or 10+ Chaff"},
  }};

  for (int i = 0; i < static_cast<int>(kRows.size()) && (my + 3 + i) < my + mh - 1; ++i) {
    mvprintw(my + 3 + i, mx + 2, "%-24s %-9s %-9s %-26s", kRows[i].name,
             kRows[i].jpn, kRows[i].kor, kRows[i].desc);
  }

  refresh();
  getch();
}

void TuiApp::showDeckInfoDialog() {
  int scroll = 0;
  while (true) {
    drawDashboard();
    int rows = 0, cols = 0;
    getmaxyx(stdscr, rows, cols);
    int mh = std::min(rows - 2, 22);
    int mw = std::min(cols - 4, 72);
    int my = (rows - mh) / 2, mx = (cols - mw) / 2;

    draw_btop_box(my, mx, mh, mw, "48-Card Hanafuda Deck Inspector",
                  "[↑/↓]Scroll [q/Esc]Close", true, CP_CYAN);

    attron(COLOR_PAIR(CP_YELLOW) | A_BOLD);
    mvprintw(my + 1, mx + 2, "%-30s %-8s %-14s %-14s", "Card", "Type", "Month",
             "Status");
    attroff(COLOR_PAIR(CP_YELLOW) | A_BOLD);

    int max_vis = mh - 3;
    scroll = std::clamp(scroll, 0, std::max(0, 48 - max_vis));

    for (int i = 0; i < max_vis && (scroll + i) < 48; ++i) {
      Card c(static_cast<std::uint8_t>(scroll + i));
      std::string status = "Unseen";
      short st_cp = CP_DIM;
      if (_gameState.players[0].has_captured(c.id())) {
        status = "You Captured";
        st_cp = CP_GREEN;
      } else if (_gameState.players[1].has_captured(c.id())) {
        status = "Com Captured";
        st_cp = CP_RED;
      } else {
        for (int s = 0; s < _gameState.num_desk_cards; ++s) {
          if (_gameState.desk_cards[s].is_valid() &&
              _gameState.desk_cards[s].id() == c.id()) {
            status = "On Table";
            st_cp = CP_YELLOW;
            break;
          }
        }
        for (const auto& hc : _gameState.players[0].hand) {
          if (hc.id() == c.id()) {
            status = "In Your Hand";
            st_cp = CP_CYAN;
            break;
          }
        }
      }

      int ry = my + 2 + i;
      short cp = card_type_color(c.type());
      attron(COLOR_PAIR(cp));
      mvprintw(ry, mx + 2, "%-30s %-8s %-14s ", card_name(c).c_str(),
               card_type_badge(c.type()).c_str(), month_name(c.month()).c_str());
      attroff(COLOR_PAIR(cp));

      attron(COLOR_PAIR(st_cp) | A_BOLD);
      printw("%-14s", status.c_str());
      attroff(COLOR_PAIR(st_cp) | A_BOLD);
    }

    refresh();
    int ch = getch();
    if (ch == 27 || ch == 'q' || ch == '\n' || ch == KEY_ENTER) break;
    if (ch == KEY_UP || ch == 'k') --scroll;
    if (ch == KEY_DOWN || ch == 'j') ++scroll;
    if (ch == KEY_PPAGE) scroll -= 10;
    if (ch == KEY_NPAGE) scroll += 10;
  }
}

void TuiApp::showHistoryDialog(int initial_item) {
  int item_idx = std::clamp(initial_item, 0, TOTAL_ITEMS - 1);
  while (true) {
    drawDashboard();
    int rows = 0, cols = 0;
    getmaxyx(stdscr, rows, cols);
    int mh = std::min(rows - 2, 20);
    int mw = std::min(cols - 4, 72);
    int my = (rows - mh) / 2, mx = (cols - mw) / 2;

    std::string title = std::format("History Chart — {}", itemName(item_idx));
    draw_btop_box(my, mx, mh, mw, title, "[←/→/c]Series [q/Esc]Close", true,
                  CP_CYAN);

    renderBrailleChart(my + 2, mx + 2, mh - 4, mw - 4, item_idx, true);
    refresh();

    int ch = getch();
    if (ch == 27 || ch == 'q' || ch == '\n' || ch == KEY_ENTER) break;
    if (ch == KEY_LEFT || ch == 'h') {
      item_idx = (item_idx - 1 + TOTAL_ITEMS) % TOTAL_ITEMS;
    } else if (ch == KEY_RIGHT || ch == 'l' || ch == 'c' || ch == ' ') {
      item_idx = (item_idx + 1) % TOTAL_ITEMS;
    }
  }
}

void TuiApp::showAboutDialog() {
  std::string msg = std::format(
      "{} v{}\n{}\n\nBased on SDLHana by Wei Mingzhi\nQt6 & NCurses TUI by {}",
      kProgramName, kProgramVersion, kProgramDescription, kProgramAuthorName);
  showMessageModal("About QHana", msg, CP_CYAN);
}

void TuiApp::showDocsDialog() {
  std::string msg =
      "Hanafuda uses 48 flower cards (12 months, 4 cards per month).\n\n"
      "1. On your turn, select a card from your hand. If its month matches a "
      "card on the table, you capture both; otherwise it is placed on the "
      "table.\n"
      "2. Next, a card is drawn from the deck and matched with the table.\n"
      "3. When you complete a Yaku (scoring combination), choose whether to "
      "STOP and claim your points or call KOI-KOI / GO to continue for higher "
      "points!";
  showMessageModal("Documentation & Rules", msg, CP_CYAN);
}

void TuiApp::showHighscoresDialog() {
  int best = _gameState.score;
  for (int s : _gameState.score_history) {
    if (s > best) best = s;
  }
  std::string msg = std::format(
      "Current Score: {} pts  |  Peak Score: {} pts\n"
      "Rounds: {} ({}W - {}L - {}D)\n\n"
      "1. Hanafuda Master — 500 pts\n"
      "2. Koi-Koi Veteran — 200 pts\n"
      "3. Go-Stop Tactician — 100 pts\n"
      "4. Tea House Regular — 50 pts",
      _gameState.score, best, _gameState.round_number, _gameState.rounds_won,
      _gameState.rounds_lost, _gameState.rounds_drawn);
  showMessageModal("High Scores & Session Stats", msg, CP_YELLOW);
}

void TuiApp::showHelpDialog() {
  std::string msg =
      "Keyboard Controls:\n"
      "  [↑/↓/←/→] or [h/j/k/l] : Select card in hand / table match\n"
      "  [1-8] or [Enter/Space] : Play selected hand card\n"
      "  [y] / [n]              : Yes (Koi-Koi / Big) / No (Stop / Small)\n"
      "  [r]                    : Start Next Round (when round is over)\n"
      "  [s]                    : Game Settings (Mode, Language, Speed)\n"
      "  [v]                    : Yaku & Scoring Reference\n"
      "  [d] or [i]             : 48-Card Deck Inspector\n"
      "  [g] / [c]              : Zoom Score Graph / Cycle Series\n"
      "  [m]                    : Toggle Sound Beep\n"
      "  [n] / [q]              : New Game / Quit";
  showMessageModal("QHana TUI Controls", msg, CP_CYAN);
}

void TuiApp::showMessageModal(const std::string& title,
                              const std::string& message, int border_color) {
  drawDashboard();
  int rows = 0, cols = 0;
  getmaxyx(stdscr, rows, cols);
  int mw = std::clamp(cols - 10, 44, 68);
  auto lines = wrap_text(message, mw - 6);
  int mh = std::clamp(static_cast<int>(lines.size()) + 5, 8, rows - 2);
  int my = (rows - mh) / 2;
  int mx = (cols - mw) / 2;

  draw_btop_box(my, mx, mh, mw, title, "[Enter/Esc]OK", true, border_color);
  for (int i = 0; i < static_cast<int>(lines.size()) && i < mh - 4; ++i) {
    mvaddstr(my + 2 + i, mx + 3, lines[i].c_str());
  }

  attron(COLOR_PAIR(CP_SELECTED) | A_BOLD);
  mvaddstr(my + mh - 2, mx + (mw - 8) / 2, " [ OK ] ");
  attroff(COLOR_PAIR(CP_SELECTED) | A_BOLD);
  refresh();

  while (true) {
    int ch = getch();
    if (ch == '\n' || ch == KEY_ENTER || ch == ' ' || ch == 27 || ch == 'q' ||
        ch == 'o') {
      break;
    }
  }
}

bool TuiApp::showConfirmModal(const std::string& title,
                              const std::string& message) {
  bool yes = true;
  while (true) {
    drawDashboard();
    int rows = 0, cols = 0;
    getmaxyx(stdscr, rows, cols);
    int mw = std::clamp(cols - 12, 42, 60);
    auto lines = wrap_text(message, mw - 6);
    int mh = static_cast<int>(lines.size()) + 6;
    int my = (rows - mh) / 2;
    int mx = (cols - mw) / 2;

    draw_btop_box(my, mx, mh, mw, title, "[y/n] [←/→]", true, CP_YELLOW);
    for (size_t i = 0; i < lines.size(); ++i) {
      mvaddstr(my + 2 + static_cast<int>(i), mx + 3, lines[i].c_str());
    }

    int btn_y = my + mh - 2;
    if (yes) attron(COLOR_PAIR(CP_SELECTED) | A_BOLD);
    mvaddstr(btn_y, mx + mw / 2 - 12, " [ Yes ] ");
    if (yes) attroff(COLOR_PAIR(CP_SELECTED) | A_BOLD);

    if (!yes) attron(COLOR_PAIR(CP_SELECTED) | A_BOLD);
    mvaddstr(btn_y, mx + mw / 2 + 3, " [ No ] ");
    if (!yes) attroff(COLOR_PAIR(CP_SELECTED) | A_BOLD);
    refresh();

    int ch = getch();
    if (ch == 'y' || ch == 'Y') return true;
    if (ch == 'n' || ch == 'N' || ch == 27) return false;
    if (ch == KEY_LEFT || ch == 'h' || ch == KEY_RIGHT || ch == 'l' ||
        ch == '\t') {
      yes = !yes;
    } else if (ch == '\n' || ch == KEY_ENTER || ch == ' ') {
      return yes;
    }
  }
}
