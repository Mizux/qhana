#include "qhana.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <format>
#include <random>
#include <unordered_map>

namespace {

enum BotHandGoal {
  HAND_MAX = 0,
  HAND_MAXNUM = 1,
  HAND_CARDS = 2,
  HAND_RIBBONS = 3,
  HAND_ANIMALS = 4,
  HAND_RED_RIBBONS = 5,
  HAND_BLUE_RIBBONS = 6,
  HAND_NORMAL_RIBBONS = 7,
  HAND_SAKECUP = 8,
  HAND_BOAR = 9,
  HAND_BIRD = 10,
  HAND_LIGHTS = 11,
  HAND_COUNT = 12,
};

int random_int(int from, int to) {
  static std::mt19937 rng(static_cast<unsigned int>(std::time(nullptr)));
  if (from >= to) return from;
  std::uniform_int_distribution<int> dist(from, to);
  return dist(rng);
}

struct TranslationEntry {
  const char* key;
  const char* eng;
  const char* fra;
  const char* jpn;
};

constexpr std::array<TranslationEntry, 45> kTranslations = {{
    {"five_lights", "Five Lights", "Cinq lumières", "五光"},
    {"four_lights", "Four Lights", "Quatre lumières", "四光"},
    {"rain_four_lights", "Rain Four Lights", "Pluie quatre lumières", "雨四光"},
    {"three_lights", "Three Lights", "Trois lumières", "三光"},
    {"rain_three_lights", "Rain Three Lights", "Pluie trois lumières", "雨三光"},
    {"red_ribbons", "Red Ribbons", "Rubans rouges", "赤短"},
    {"blue_ribbons", "Blue Ribbons", "Rubans bleus", "青短"},
    {"normal_ribbons", "Grass Ribbons", "Rubans normaux", "草短"},
    {"ribbons", "Ribbons", "Rubans", "短札"},
    {"boar_deer_butterfly", "Boar, Deer & Butterfly", "Sanglier, Cerf & Papillon", "猪鹿蝶"},
    {"five_birds", "Five Birds", "Cinq oiseaux", "五鳥"},
    {"animals", "Animals", "Animaux", "タネ"},
    {"flower_meets_sakecup", "Flower Meets Sakecup", "La fleur et la coupe de saké", "花見酒"},
    {"moon_meets_sakecup", "Moon Meets Sakecup", "La lune et la coupe de saké", "月見酒"},
    {"cards", "Cards", "Cartes", "カス"},
    {"dealer", "Dealer", "Distributeur", "親権"},
    {"point", "Point", "Point", "文"},
    {"points", "Points", "Points", "文"},
    {"koikoiyesorno", "Would you like to continue the game (Koi-Koi)?",
     "Voulez-vous continuer la partie (Koi-Koi) ?", "こいこいしますか?"},
    {"go", "Yes (Koi-Koi)", "Oui", "はい"},
    {"stop", "No (Stop)", "Non", "いいえ"},
    {"comdealer", "Computer is the dealer this round.", "Je distribue ce tour-ci.",
     "わたしが親です。"},
    {"youdealer", "You are the dealer this round.", "Vous distribuez ce tour-ci.",
     "あなたが親です。"},
    {"discardcardselect", "Please select a card from your hand.",
     "Veuillez sélectionner une carte.", "札を切って下さい。"},
    {"deskcardselect", "Please select a matching card on the table.",
     "Veuillez choisir une carte sur la table.", "場の札を選んで下さい。"},
    {"computerwin", "You Lose!", "Vous avez perdu !", "あなたの負けです。"},
    {"youwin", "You Win!", "Vous avez gagné !", "あなたの勝ちです。"},
    {"drawgame", "Round Draw!", "Match nul !", "引き分け"},
    {"total", "Total", "Total", "合計"},
    {"play", "Start Game", "Commencer une partie", "GAME START"},
    {"settings", "Game Settings", "Paramètres du jeu", "ゲームの設定"},
    {"quit", "Quit Game", "Quitter la partie", "終了"},
    {"doubleupyesorno", "Do you want to Double-Up?", "Voulez-vous doubler ?", "W-UP しますか?"},
    {"yes", "Yes", "Oui", "はい"},
    {"no", "No", "Non", "いいえ"},
    {"bigorsmall", "Big (Jul-Dec) or Small (Jan-Jun)?", "Gros ou petit ?", "BIG or SMALL?"},
    {"big", "Big (7-12)", "Gros (7-12)", "大 (7-12)"},
    {"small", "Small (1-6)", "Petit (1-6)", "小 (1-6)"},
    {"gamemode0", "Koi-Koi", "Koi-Koi", "こいこい"},
    {"gamemode1", "Koi-Koi [BET]", "Koi-Koi [BET]", "こいこい [BET]"},
    {"gamemode2", "Go-Stop", "Go-Stop", "五鳥 (Go-Stop)"},
    {"comget3pts", "You lost 3 points! (First turn Ppeok)", "Vous perdez 3 points !",
     "相手に3文取られました!"},
    {"youget3pts", "You got 3 points! (First turn Ppeok)", "Vous gagnez 3 points !",
     "3文獲得しました!"},
    {"comget5pts", "You lost 3 points! (Three Ppeoks)", "Vous perdez 3 points !",
     "三連ペクで負け!"},
    {"youget5pts", "You got 3 points! (Three Ppeoks)", "Vous gagnez 3 points !",
     "三連ペクで勝ち!"},
}};

}  // namespace

CardType Card::type() const {
  switch (id_) {
    case 0:
    case 8:
    case 28:
    case 40:
    case 44:
      return CardType::Light;

    case 4:
    case 12:
    case 16:
    case 20:
    case 24:
    case 29:
    case 32:
    case 36:
    case 41:
      return CardType::Animal;

    case 13:
    case 17:
    case 25:
    case 42:
      return CardType::Ribbon;

    case 1:
    case 5:
    case 9:
      return CardType::RibbonRed;

    case 21:
    case 33:
    case 37:
      return CardType::RibbonBlue;

    default:
      return CardType::None;
  }
}

void PlayerState::reset() {
  hand.clear();
  captured.clear();
  result = YakuResult{};
  prev_result = YakuResult{};
  num_continue = 0;
  num_leave_three = 0;
}

bool PlayerState::has_captured(std::uint8_t card_id) const {
  for (const auto& c : captured) {
    if (c.id() == card_id) return true;
  }
  return false;
}

std::vector<Card> PlayerState::captured_special() const {
  std::vector<Card> spec;
  for (const auto& c : captured) {
    if (c.type() != CardType::None) {
      spec.push_back(c);
    }
  }
  std::stable_sort(spec.begin(), spec.end(), [](const Card& a, const Card& b) {
    if (a.order() != b.order()) return a.order() > b.order();
    return a.id() < b.id();
  });
  return spec;
}

std::vector<Card> PlayerState::captured_normal() const {
  std::vector<Card> norm;
  for (const auto& c : captured) {
    if (c.type() == CardType::None) {
      norm.push_back(c);
    }
  }
  return norm;
}

int PlayerState::count_lights() const {
  int n = 0;
  for (const auto& c : captured) {
    if (c.type() == CardType::Light) ++n;
  }
  return n;
}

int PlayerState::count_animals() const {
  int n = 0;
  for (const auto& c : captured) {
    if (c.type() == CardType::Animal) ++n;
  }
  return n;
}

int PlayerState::count_ribbons() const {
  int n = 0;
  for (const auto& c : captured) {
    auto t = c.type();
    if (t == CardType::Ribbon || t == CardType::RibbonRed ||
        t == CardType::RibbonBlue) {
      ++n;
    }
  }
  return n;
}

int PlayerState::count_chaff(GameMode mode) const {
  int n = 0;
  for (const auto& c : captured) {
    if (c.type() == CardType::None) {
      ++n;
      if (mode == GameMode::Korean && c.is_double_chaff()) {
        ++n;
      }
    } else if (c.is_sake_cup()) {
      n += (mode == GameMode::Korean) ? 2 : 1;
    }
  }
  return n;
}

std::string month_name(int month) {
  constexpr std::array<const char*, 12> kMonths = {
      "Jan (Pine)",     "Feb (Plum)",    "Mar (Cherry)",  "Apr (Wisteria)",
      "May (Iris)",     "Jun (Peony)",   "Jul (Clover)",  "Aug (Susuki)",
      "Sep (Kiku)",     "Oct (Maple)",   "Nov (Willow)",  "Dec (Paulownia)"};
  if (month >= 1 && month <= 12) return kMonths[month - 1];
  return "Unknown";
}

std::string month_flower_name(int month) {
  constexpr std::array<const char*, 12> kFlowers = {
      "Pine",  "Plum",   "Cherry", "Wisteria", "Iris",  "Peony",
      "Clover","Susuki", "Kiku",   "Maple",    "Willow","Paulownia"};
  if (month >= 1 && month <= 12) return kFlowers[month - 1];
  return "Back";
}

std::string card_name(const Card& c) {
  if (!c.is_valid()) return "Card Back";
  constexpr std::array<const char*, 48> kNames = {
      // Month 1: Pine (Matsu)
      "1-Jan: Pine with Crane & Sun",
      "1-Jan: Pine Poetry Ribbon",
      "1-Jan: Pine Chaff 1",
      "1-Jan: Pine Chaff 2",
      // Month 2: Plum (Ume)
      "2-Feb: Plum with Bush Warbler",
      "2-Feb: Plum Poetry Ribbon",
      "2-Feb: Plum Chaff 1",
      "2-Feb: Plum Chaff 2",
      // Month 3: Cherry (Sakura)
      "3-Mar: Cherry Curtain",
      "3-Mar: Cherry Poetry Ribbon",
      "3-Mar: Cherry Chaff 1",
      "3-Mar: Cherry Chaff 2",
      // Month 4: Wisteria (Fuji)
      "4-Apr: Wisteria with Cuckoo",
      "4-Apr: Wisteria Red Ribbon",
      "4-Apr: Wisteria Chaff 1",
      "4-Apr: Wisteria Chaff 2",
      // Month 5: Iris (Ayame)
      "5-May: Iris Eight-Plank Bridge",
      "5-May: Iris Red Ribbon",
      "5-May: Iris Chaff 1",
      "5-May: Iris Chaff 2",
      // Month 6: Peony (Botan)
      "6-Jun: Peony with Butterflies",
      "6-Jun: Peony Blue Ribbon",
      "6-Jun: Peony Chaff 1",
      "6-Jun: Peony Chaff 2",
      // Month 7: Bush Clover (Hagi)
      "7-Jul: Clover with Boar",
      "7-Jul: Clover Red Ribbon",
      "7-Jul: Clover Chaff 1",
      "7-Jul: Clover Chaff 2",
      // Month 8: Susuki Grass
      "8-Aug: Susuki Full Moon",
      "8-Aug: Susuki with Geese",
      "8-Aug: Susuki Chaff 1",
      "8-Aug: Susuki Chaff 2",
      // Month 9: Chrysanthemum (Kiku)
      "9-Sep: Chrysanthemum Sake Cup",
      "9-Sep: Chrysanthemum Blue Ribbon",
      "9-Sep: Chrysanthemum Chaff 1",
      "9-Sep: Chrysanthemum Chaff 2",
      // Month 10: Maple (Momiji)
      "10-Oct: Maple with Deer",
      "10-Oct: Maple Blue Ribbon",
      "10-Oct: Maple Chaff 1",
      "10-Oct: Maple Chaff 2",
      // Month 11: Willow (Yanagi)
      "11-Nov: Willow Rain Man",
      "11-Nov: Willow with Swallow",
      "11-Nov: Willow Red Ribbon",
      "11-Nov: Willow Lightning",
      // Month 12: Paulownia (Kiri)
      "12-Dec: Paulownia Phoenix",
      "12-Dec: Paulownia Yellow Chaff",
      "12-Dec: Paulownia Chaff 2",
      "12-Dec: Paulownia Chaff 3",
  };
  return kNames[c.id()];
}

std::string card_short_label(const Card& c) {
  if (!c.is_valid()) return "??";
  switch (c.id()) {
    case 0: return "Crane";
    case 1: return "Poet";
    case 4: return "Warbl";
    case 5: return "Poet";
    case 8: return "Curtn";
    case 9: return "Poet";
    case 12: return "Cucko";
    case 13: return "RedRb";
    case 16: return "Bridg";
    case 17: return "RedRb";
    case 20: return "Btrfl";
    case 21: return "BluRb";
    case 24: return "Boar";
    case 25: return "RedRb";
    case 28: return "Moon";
    case 29: return "Geese";
    case 32: return "Sake";
    case 33: return "BluRb";
    case 36: return "Deer";
    case 37: return "BluRb";
    case 40: return "Rain";
    case 41: return "Swllw";
    case 42: return "RedRb";
    case 43: return "Storm";
    case 44: return "Phnix";
    case 45: return "Kiri2";
    default: return "Chaff";
  }
}

std::string card_type_name(CardType type) {
  switch (type) {
    case CardType::Light:
      return "Light";
    case CardType::Animal:
      return "Animal";
    case CardType::RibbonBlue:
      return "Blue Ribbon";
    case CardType::RibbonRed:
      return "Poetry Ribbon";
    case CardType::Ribbon:
      return "Red Ribbon";
    case CardType::None:
      return "Chaff";
  }
  return "Chaff";
}

std::string card_type_badge(CardType type) {
  switch (type) {
    case CardType::Light:
      return "★LGT";
    case CardType::Animal:
      return "◆ANI";
    case CardType::RibbonBlue:
      return "▬BLU";
    case CardType::RibbonRed:
      return "▬RED";
    case CardType::Ribbon:
      return "▬RIB";
    case CardType::None:
      return "·CHF";
  }
  return "·CHF";
}

std::string game_mode_name(GameMode mode, Language lang) {
  switch (mode) {
    case GameMode::KoiKoi:
      return tr_msg("gamemode0", lang);
    case GameMode::Bet:
      return tr_msg("gamemode1", lang);
    case GameMode::Korean:
      return tr_msg("gamemode2", lang);
  }
  return "Koi-Koi";
}

std::string language_name(Language lang, Language /*display_lang*/) {
  switch (lang) {
    case Language::English:
      return "English";
    case Language::French:
      return "Français";
    case Language::Japanese:
      return "日本語";
  }
  return "English";
}

std::string anim_speed_name(AnimSpeed speed, Language /*lang*/) {
  switch (speed) {
    case AnimSpeed::VerySlow:
      return "Very Slow";
    case AnimSpeed::Slow:
      return "Slow";
    case AnimSpeed::Middle:
      return "Middle";
    case AnimSpeed::Fast:
      return "Fast";
    case AnimSpeed::VeryFast:
      return "Very Fast";
  }
  return "Fast";
}

int anim_speed_ms(AnimSpeed speed) {
  switch (speed) {
    case AnimSpeed::VerySlow:
      return 800;
    case AnimSpeed::Slow:
      return 500;
    case AnimSpeed::Middle:
      return 300;
    case AnimSpeed::Fast:
      return 180;
    case AnimSpeed::VeryFast:
      return 50;
  }
  return 180;
}

std::string tr_msg(std::string_view key, Language lang) {
  for (const auto& entry : kTranslations) {
    if (key == entry.key) {
      switch (lang) {
        case Language::French:
          return entry.fra;
        case Language::Japanese:
          return entry.jpn;
        case Language::English:
        default:
          return entry.eng;
      }
    }
  }
  return std::string(key);
}

GameState::GameState() {
  players[0].is_bot = false;
  players[1].is_bot = true;
  new_game();
}

void GameState::add_log(const std::string& entry) {
  game_log.push_back(entry);
  if (game_log.size() > 200) {
    game_log.erase(game_log.begin());
  }
}

void GameState::reset_deck() { card_flags_.fill(0); }

int GameState::remaining_deck_count() const {
  int count = 0;
  for (int i = 0; i < 48; ++i) {
    if (!(card_flags_[i / 8] & (1 << (i & 7)))) {
      ++count;
    }
  }
  return count;
}

Card GameState::draw_random_card() {
  std::array<std::uint8_t, 48> available{};
  int count = 0;
  for (int i = 0; i < 48; ++i) {
    if (!(card_flags_[i / 8] & (1 << (i & 7)))) {
      available[count++] = static_cast<std::uint8_t>(i);
    }
  }
  if (count <= 0) {
    return Card(255);
  }
  int pick = random_int(0, count - 1);
  std::uint8_t card_id = available[pick];
  card_flags_[card_id / 8] |= (1 << (card_id & 7));
  return Card(card_id);
}

void GameState::put_back_to_pile(const Card& c) {
  if (!c.is_valid()) return;
  auto val = c.id();
  card_flags_[val / 8] &= ~(1 << (val & 7));
}

int GameState::find_free_desk_slot(int exclude) const {
  int i = 0;
  for (i = 0; i < num_desk_cards; ++i) {
    if (exclude != -1 && i == exclude) continue;
    if (!desk_cards[i].is_valid() && i != exclude) {
      return i;
    }
  }
  return i;
}

std::vector<int> GameState::matching_desk_slots(int month) const {
  std::vector<int> slots;
  for (int i = 0; i < num_desk_cards; ++i) {
    if (desk_cards[i].is_valid() && desk_cards[i].month() == month) {
      slots.push_back(i);
    }
  }
  return slots;
}

std::vector<Card> GameState::valid_desk_cards() const {
  std::vector<Card> res;
  for (int i = 0; i < num_desk_cards; ++i) {
    if (desk_cards[i].is_valid()) res.push_back(desk_cards[i]);
  }
  return res;
}

void GameState::new_game() {
  score = 0;
  round_number = 0;
  rounds_won = 0;
  rounds_lost = 0;
  rounds_drawn = 0;
  score_history.clear();
  player_round_scores.clear();
  bot_round_scores.clear();
  game_log.clear();

  if (mode != GameMode::Bet) {
    dealer = random_int(0, 1);
  } else {
    dealer = 1;
  }

  new_round();
}

void GameState::new_round() {
  reset_deck();
  num_desk_cards = 0;
  for (auto& c : desk_cards) c.destroy();

  ++round_number;
  if (mode != GameMode::Bet) {
    if (round_number > 1) {
      dealer = 1 - dealer;
    }
  } else {
    dealer = 1;
  }

  players[0].reset();
  players[1].reset();
  players[0].is_bot = false;
  players[1].is_bot = true;

  int hand_size = max_hand_cards();

  // Deal hand cards to both players with anti-4-of-a-kind and anti-4-pairs check
  for (int p = 0; p < 2; ++p) {
    players[p].hand.resize(hand_size);
    for (int i = 0; i < hand_size; ++i) {
      players[p].hand[i] = draw_random_card();
    }

    bool allpairs = true;
    for (int i = 0; i < hand_size; ++i) {
      int count = 0;
      for (int j = 0; j < hand_size; ++j) {
        if (players[p].hand[i] == players[p].hand[j]) {
          ++count;
        }
      }
      if (count != 2) {
        allpairs = false;
      }
      if (count >= 4) {
        int count2 = 999;
        while (count2 >= 4) {
          put_back_to_pile(players[p].hand[i]);
          players[p].hand[i] = draw_random_card();
          count2 = 0;
          for (int k = 0; k < hand_size; ++k) {
            if (players[p].hand[i] == players[p].hand[k]) {
              ++count2;
            }
          }
        }
      }
    }

    if (allpairs) {
      int index = random_int(0, hand_size - 1);
      int count = 999;
      while (count >= 2) {
        put_back_to_pile(players[p].hand[index]);
        players[p].hand[index] = draw_random_card();
        count = 0;
        for (int k = 0; k < hand_size; ++k) {
          if (players[p].hand[index] == players[p].hand[k]) {
            ++count;
          }
        }
      }
    }
  }

  // Deal 8 cards to the table
  for (int i = 0; i < 8; ++i) {
    desk_cards[i] = draw_random_card();
  }
  num_desk_cards = 8;

  // Don't allow 3 or more of the same month or 4 pairs on the table
  bool allpairs = true;
  for (int i = 0; i < num_desk_cards; ++i) {
    int count = 0;
    for (int j = 0; j < num_desk_cards; ++j) {
      if (desk_cards[i] == desk_cards[j]) {
        ++count;
      }
    }
    if (count != 2) {
      allpairs = false;
    }
    if (count >= 3) {
      int count2 = 999;
      while (count2 >= 3) {
        put_back_to_pile(desk_cards[i]);
        desk_cards[i] = draw_random_card();
        count2 = 0;
        for (int k = 0; k < num_desk_cards; ++k) {
          if (desk_cards[i] == desk_cards[k]) {
            ++count2;
          }
        }
      }
    }
  }

  if (allpairs) {
    int index = random_int(0, num_desk_cards - 1);
    int count = 999;
    while (count >= 2) {
      put_back_to_pile(desk_cards[index]);
      desk_cards[index] = draw_random_card();
      count = 0;
      for (int k = 0; k < num_desk_cards; ++k) {
        if (desk_cards[index] == desk_cards[k]) {
          ++count;
        }
      }
    }
  }

  if (mode == GameMode::Bet) {
    score -= 1;
  }
  score = std::clamp(score, -99999, 99999);

  current_player = dealer;
  winner = -1;
  phase = GamePhase::SelectHandCard;
  last_played_card.destroy();
  last_drawn_card.destroy();
  double_up_card.destroy();
  double_up_won = false;
  pending_getfour_month = -1;
  pending_leavethree = false;
  pending_old_slot = 999;
  desk_choice_slots = {-1, -1};
  last_formed_yaku.clear();
  round_yaku_summary.clear();

  status_banner = (dealer == 1) ? msg("comdealer") : msg("youdealer");
  add_log(std::format("--- Round {} ({}) ---", round_number,
                      game_mode_name(mode, language)));
  add_log(status_banner);
}

bool GameState::play_hand_card(int hand_idx) {
  if (phase != GamePhase::SelectHandCard) return false;
  auto& cur = players[current_player];
  if (hand_idx < 0 || hand_idx >= static_cast<int>(cur.hand.size())) {
    return false;
  }

  clear_card_effects();
  Card c = cur.hand[hand_idx];
  cur.hand.erase(cur.hand.begin() + hand_idx);
  last_played_card = c;

  // Draw one card from the deck for step 2 of this turn
  last_drawn_card = draw_random_card();
  pending_getfour_month = -1;
  pending_leavethree = false;
  pending_old_slot = 999;

  std::string who = (current_player == 0) ? "You" : "Computer";
  add_log(std::format("{} played [{}].", who, card_name(c)));

  // Check matching cards on desk
  auto matches = matching_desk_slots(c.month());
  int count = static_cast<int>(matches.size());

  if (count <= 0) {
    int slot = find_free_desk_slot();
    desk_cards[slot] = c;
    if (slot >= num_desk_cards) {
      num_desk_cards = slot + 1;
    }
    pending_old_slot = 999;
    if (mode == GameMode::Korean) {
      pending_getfour_month = c.month();
    }
  } else if (count == 1) {
    int slot = matches[0];
    // Notice: cur.hand already had 1 card removed, so > 0 means > 1 before removal
    if (mode == GameMode::Korean && c == last_drawn_card &&
        !cur.hand.empty()) {
      int slot1 = find_free_desk_slot();
      desk_cards[slot1] = c;
      if (slot1 >= num_desk_cards) {
        num_desk_cards = slot1 + 1;
      }
      pending_leavethree = true;
      pending_old_slot = 999;
      add_log(std::format("{} triggered Ppeok (3 cards of Month {} on table)!",
                          who, c.month()));
    } else {
      add_log(std::format("{} captured [{}] with [{}].", who,
                          card_name(desk_cards[slot]), card_name(c)));
      cur.captured.push_back(c);
      cur.captured.push_back(desk_cards[slot]);
      desk_cards[slot].destroy();
      pending_old_slot = slot;
    }
  } else if (mode == GameMode::Korean && count >= 3) {
    cur.captured.push_back(c);
    for (int k = 0; k < 3; ++k) {
      cur.captured.push_back(desk_cards[matches[k]]);
      desk_cards[matches[k]].destroy();
    }
    pending_old_slot = 999;
    add_log(std::format("{} swept all 4 cards of Month {}!", who, c.month()));
    if (!cur.hand.empty() && !players[1 - current_player].hand.empty()) {
      steal_one_chaff_from_opponent(current_player);
    }
  } else if (count >= 2) {
    int i0 = matches[0];
    int i1 = matches[1];
    int chosen_slot = -1;
    if (desk_cards[i0].type() == desk_cards[i1].type()) {
      if (desk_cards[i1].is_double_chaff()) {
        chosen_slot = i1;
      } else {
        chosen_slot = i0;
      }
    } else if (current_player == 1) {
      chosen_slot = bot_select_desk_card(desk_cards[i0].month(), c);
    } else {
      // Human must choose which desk card to capture
      desk_choice_slots = {i0, i1};
      desk_cards[i0].render_effect |= static_cast<unsigned int>(CardEffect::Box);
      desk_cards[i1].render_effect |= static_cast<unsigned int>(CardEffect::Box);
      phase = GamePhase::SelectDeskCardForHand;
      status_banner = msg("deskcardselect");
      return true;
    }

    add_log(std::format("{} captured [{}] with [{}].", who,
                        card_name(desk_cards[chosen_slot]), card_name(c)));
    cur.captured.push_back(c);
    cur.captured.push_back(desk_cards[chosen_slot]);
    desk_cards[chosen_slot].destroy();
    pending_old_slot = chosen_slot;
    if (mode == GameMode::Korean) {
      pending_getfour_month = c.month();
    }
  }

  resolve_drawn_card_step();
  return true;
}

bool GameState::select_desk_card(int desk_slot) {
  if (phase != GamePhase::SelectDeskCardForHand &&
      phase != GamePhase::SelectDeskCardForDrawn) {
    return false;
  }
  if (desk_slot != desk_choice_slots[0] && desk_slot != desk_choice_slots[1]) {
    return false;
  }

  clear_card_effects();
  auto& cur = players[current_player];
  std::string who = (current_player == 0) ? "You" : "Computer";

  if (phase == GamePhase::SelectDeskCardForHand) {
    add_log(std::format("{} captured [{}] with [{}].", who,
                        card_name(desk_cards[desk_slot]),
                        card_name(last_played_card)));
    cur.captured.push_back(last_played_card);
    cur.captured.push_back(desk_cards[desk_slot]);
    desk_cards[desk_slot].destroy();
    pending_old_slot = desk_slot;
    if (mode == GameMode::Korean) {
      pending_getfour_month = last_played_card.month();
    }
    desk_choice_slots = {-1, -1};
    resolve_drawn_card_step();
  } else {
    add_log(std::format("{} captured [{}] with drawn [{}].", who,
                        card_name(desk_cards[desk_slot]),
                        card_name(last_drawn_card)));
    cur.captured.push_back(desk_cards[desk_slot]);
    cur.captured.push_back(last_drawn_card);
    desk_cards[desk_slot].destroy();
    desk_choice_slots = {-1, -1};
    finish_turn_after_captures();
  }
  return true;
}

void GameState::resolve_drawn_card_step() {
  auto& cur = players[current_player];
  std::string who = (current_player == 0) ? "You" : "Computer";

  if (!last_drawn_card.is_valid()) {
    finish_turn_after_captures();
    return;
  }

  add_log(std::format("{} drew [{}] from the pile.", who,
                      card_name(last_drawn_card)));

  auto matches = matching_desk_slots(last_drawn_card.month());
  int count = static_cast<int>(matches.size());

  if (count <= 0 || pending_leavethree) {
    int slot = find_free_desk_slot(pending_old_slot);
    desk_cards[slot] = last_drawn_card;
    if (slot >= num_desk_cards) {
      num_desk_cards = slot + 1;
    }

    if (pending_leavethree) {
      cur.num_leave_three++;
      // Note: cur.hand already had 1 card removed, so >= 7 means first round (8 cards initially)
      if (static_cast<int>(cur.hand.size()) >= 7) {
        if (current_player == 1) {
          status_banner = msg("comget3pts");
          score -= 3;
        } else {
          status_banner = msg("youget3pts");
          score += 3;
        }
        add_log(status_banner);
      }
    }
  } else if (count == 1) {
    int slot = matches[0];
    if (last_drawn_card.month() == pending_getfour_month) {
      if (!cur.hand.empty() && !players[1 - current_player].hand.empty()) {
        steal_one_chaff_from_opponent(current_player);
      }
    }
    add_log(std::format("{} captured [{}] with drawn [{}].", who,
                        card_name(desk_cards[slot]),
                        card_name(last_drawn_card)));
    cur.captured.push_back(desk_cards[slot]);
    cur.captured.push_back(last_drawn_card);
    desk_cards[slot].destroy();
  } else if (mode == GameMode::Korean && count >= 3) {
    cur.captured.push_back(last_drawn_card);
    for (int k = 0; k < 3; ++k) {
      cur.captured.push_back(desk_cards[matches[k]]);
      desk_cards[matches[k]].destroy();
    }
    add_log(std::format("{} swept all 4 cards of Month {} with drawn card!",
                        who, last_drawn_card.month()));
    if (!cur.hand.empty() && !players[1 - current_player].hand.empty()) {
      steal_one_chaff_from_opponent(current_player);
    }
  } else if (count >= 2) {
    int i0 = matches[0];
    int i1 = matches[1];
    int chosen_slot = -1;
    if (desk_cards[i0].type() == desk_cards[i1].type()) {
      if (desk_cards[i1].is_double_chaff()) {
        chosen_slot = i1;
      } else {
        chosen_slot = i0;
      }
    } else if (current_player == 1) {
      chosen_slot =
          bot_select_desk_card(desk_cards[i0].month(), last_drawn_card);
    } else {
      desk_choice_slots = {i0, i1};
      desk_cards[i0].render_effect |= static_cast<unsigned int>(CardEffect::Box);
      desk_cards[i1].render_effect |= static_cast<unsigned int>(CardEffect::Box);
      phase = GamePhase::SelectDeskCardForDrawn;
      status_banner = msg("deskcardselect");
      return;
    }

    add_log(std::format("{} captured [{}] with drawn [{}].", who,
                        card_name(desk_cards[chosen_slot]),
                        card_name(last_drawn_card)));
    cur.captured.push_back(desk_cards[chosen_slot]);
    cur.captured.push_back(last_drawn_card);
    desk_cards[chosen_slot].destroy();
  }

  finish_turn_after_captures();
}

void GameState::steal_one_chaff_from_opponent(int player_idx) {
  auto& cur = players[player_idx];
  auto& opn = players[1 - player_idx];

  int index = -1;
  int index2 = -1;
  for (int i = 0; i < static_cast<int>(opn.captured.size()); ++i) {
    const Card& c = opn.captured[i];
    if (c.type() != CardType::None) continue;
    if (c.is_double_chaff()) {
      index2 = i;
    } else {
      index = i;
    }
  }

  int pick = (index != -1) ? index : index2;
  if (pick == -1) return;

  Card stolen = opn.captured[pick];
  cur.captured.push_back(stolen);
  opn.captured.erase(opn.captured.begin() + pick);

  std::string who = (player_idx == 0) ? "You" : "Computer";
  add_log(std::format("{} stole chaff card [{}] from opponent!", who,
                      card_name(stolen)));
}

void GameState::finish_turn_after_captures() {
  auto& cur = players[current_player];
  auto& opn = players[1 - current_player];

  // Korean Pan-sseuri: if table is completely cleared, steal 1 chaff from opponent
  if (mode == GameMode::Korean) {
    bool any_valid = false;
    for (int i = 0; i < num_desk_cards; ++i) {
      if (desk_cards[i].is_valid()) {
        any_valid = true;
        break;
      }
    }
    if (!any_valid && !cur.hand.empty() && !opn.hand.empty()) {
      steal_one_chaff_from_opponent(current_player);
    }
  }

  calc_result(0);
  calc_result(1);

  // Check if current player formed a new or higher yaku
  if (cur.result.score - cur.prev_result.score > 0 &&
      (mode != GameMode::Korean || cur.result.score >= 3)) {
    winner = current_player;
    last_formed_yaku = get_yaku_items(current_player, true);

    std::string who = (current_player == 0) ? "You" : "Computer";
    for (const auto& y : last_formed_yaku) {
      add_log(std::format("{} formed Yaku: {} ({} pts)!", who, y.name,
                          y.points));
    }

    if (!cur.hand.empty()) {
      if (current_player == 0) {
        phase = GamePhase::AskKoiKoi;
        status_banner = msg("koikoiyesorno");
        return;
      } else {
        if (bot_want_to_continue()) {
          cur.prev_result = cur.result;
          cur.num_continue++;
          status_banner = "Computer declared Koi-Koi / Go!";
          add_log(status_banner);
        } else {
          finalize_round();
          return;
        }
      }
    } else {
      finalize_round();
      return;
    }
  }

  // Korean 3x Leave-Three (Sam-ppeok) instant win
  if (mode == GameMode::Korean && cur.num_leave_three >= 3) {
    if (current_player == 1) {
      status_banner = msg("comget5pts");
      score -= 3;
      rounds_lost++;
      winner = 1;
    } else {
      status_banner = msg("youget5pts");
      score += 3;
      rounds_won++;
      winner = 0;
    }
    score = std::clamp(score, -99999, 99999);
    score_history.push_back(score);
    player_round_scores.push_back(players[0].result.score);
    bot_round_scores.push_back(players[1].result.score);
    add_log(status_banner);
    phase = GamePhase::RoundOver;
    return;
  }

  // Switch turn to opponent
  current_player = 1 - current_player;

  // Check if next player has no cards left in hand
  if (players[current_player].hand.empty()) {
    if (mode != GameMode::Korean) {
      calc_result(current_player);
      calc_result(1 - current_player);
      if (players[current_player].result.score > 0 &&
          players[1 - current_player].result.score <= 0) {
        winner = current_player;
      } else if (players[1 - current_player].result.score > 0 &&
                 players[current_player].result.score <= 0) {
        winner = 1 - current_player;
      } else {
        winner = -1;
      }
    } else {
      winner = -1;
    }
    finalize_round();
    return;
  }

  phase = GamePhase::SelectHandCard;
  status_banner = (current_player == 0) ? msg("discardcardselect")
                                        : "Computer is thinking...";
}

void GameState::answer_koikoi(bool continue_game) {
  if (phase != GamePhase::AskKoiKoi) return;
  auto& cur = players[current_player];

  if (continue_game) {
    cur.prev_result = cur.result;
    cur.num_continue++;
    add_log("You declared Koi-Koi / Go!");

    if (mode == GameMode::Korean && cur.num_leave_three >= 3) {
      status_banner = msg("youget5pts");
      score += 3;
      score = std::clamp(score, -99999, 99999);
      rounds_won++;
      score_history.push_back(score);
      player_round_scores.push_back(players[0].result.score);
      bot_round_scores.push_back(players[1].result.score);
      phase = GamePhase::RoundOver;
      return;
    }

    current_player = 1 - current_player;
    if (players[current_player].hand.empty()) {
      finalize_round();
      return;
    }
    phase = GamePhase::SelectHandCard;
    status_banner = (current_player == 0) ? msg("discardcardselect")
                                          : "Computer is thinking...";
  } else {
    add_log("You chose to Stop and claim the round!");
    finalize_round();
  }
}

void GameState::finalize_round() {
  if (winner != -1) {
    if (mode == GameMode::Korean) {
      calc_add_result(winner);
    }
    round_yaku_summary = get_yaku_items(winner, false);

    if (winner == 0 && mode == GameMode::Bet &&
        players[0].result.score > 0) {
      phase = GamePhase::AskDoubleUp;
      status_banner = msg("doubleupyesorno");
      return;
    }

    if (winner == 1) {
      if (mode != GameMode::Bet) {
        score -= players[1].result.score;
      }
      rounds_lost++;
      status_banner = std::format("{} ({}: {} {})", msg("computerwin"),
                                  msg("total"), players[1].result.score,
                                  (players[1].result.score <= 1) ? msg("point")
                                                                 : msg("points"));
    } else {
      score += players[0].result.score;
      rounds_won++;
      status_banner = std::format("{} ({}: {} {})", msg("youwin"), msg("total"),
                                  players[0].result.score,
                                  (players[0].result.score <= 1) ? msg("point")
                                                                 : msg("points"));
    }
  } else {
    rounds_drawn++;
    round_yaku_summary.clear();
    status_banner = msg("drawgame");
  }

  score = std::clamp(score, -99999, 99999);
  score_history.push_back(score);
  player_round_scores.push_back(players[0].result.score);
  bot_round_scores.push_back(players[1].result.score);
  add_log(status_banner);
  phase = GamePhase::RoundOver;
}

void GameState::answer_double_up(bool want_double) {
  if (phase != GamePhase::AskDoubleUp) return;
  if (!want_double) {
    score += players[0].result.score;
    score = std::clamp(score, -99999, 99999);
    rounds_won++;
    score_history.push_back(score);
    player_round_scores.push_back(players[0].result.score);
    bot_round_scores.push_back(players[1].result.score);
    status_banner = std::format("{} ({}: {} {})", msg("youwin"), msg("total"),
                                players[0].result.score,
                                (players[0].result.score <= 1) ? msg("point")
                                                               : msg("points"));
    add_log(status_banner);
    phase = GamePhase::RoundOver;
    return;
  }

  phase = GamePhase::AskBigOrSmall;
  status_banner = msg("bigorsmall");
}

void GameState::answer_big_or_small(bool is_big) {
  if (phase != GamePhase::AskBigOrSmall) return;
  double_up_card = Card(static_cast<std::uint8_t>(random_int(0, 47)));

  bool won = (is_big && double_up_card.month() >= 7) ||
             (!is_big && double_up_card.month() < 7);
  double_up_won = won;

  if (won) {
    players[0].result.score *= 2;
    status_banner = std::format(
        "Double-Up WIN! Drawn [{}] (Month {}). Score doubled to {}! {}",
        card_name(double_up_card), double_up_card.month(),
        players[0].result.score, msg("doubleupyesorno"));
    add_log(status_banner);
    phase = GamePhase::AskDoubleUp;
  } else {
    players[0].result.score = 0;
    status_banner = std::format(
        "Double-Up LOSE! Drawn [{}] (Month {}). Winnings lost.",
        card_name(double_up_card), double_up_card.month());
    add_log(status_banner);
    rounds_lost++;
    score_history.push_back(score);
    player_round_scores.push_back(0);
    bot_round_scores.push_back(players[1].result.score);
    phase = GamePhase::RoundOver;
  }
}

void GameState::step_bot() {
  if (phase == GamePhase::SelectHandCard && current_player == 1) {
    int hand_idx = bot_select_hand_card();
    play_hand_card(hand_idx);
  }
}

void GameState::calc_result(int player_idx) {
  auto& p = players[player_idx];
  auto& opn = players[1 - player_idx];
  p.result = YakuResult{};

  int num_lights = 0, num_animals = 0, num_ribbons = 0;
  int num_red = 0, num_blue = 0, num_grass = 0;
  int num_cards = 0, num_birds = 0;

  bool has_rain = false;
  bool has_boar = false, has_deer = false, has_butterfly = false;
  bool has_sakecup = false, has_moon = false, has_flower = false;

  for (const auto& c : p.captured) {
    auto type = c.type();
    if (type == CardType::Light) {
      num_lights++;
      if (c.is_rain()) {
        has_rain = true;
      } else if (c.is_moon()) {
        has_moon = true;
      } else if (c.is_flower()) {
        has_flower = true;
      }
    } else if (type == CardType::RibbonRed) {
      num_red++;
      num_ribbons++;
    } else if (type == CardType::RibbonBlue) {
      num_blue++;
      num_ribbons++;
    } else if (type == CardType::Ribbon) {
      num_ribbons++;
      if (c.month() != 11) {
        num_grass++;
      }
    } else if (type == CardType::Animal) {
      num_animals++;
      if (c.is_sake_cup()) {
        num_cards += (mode == GameMode::Korean) ? 2 : 1;
        has_sakecup = true;
      } else if (c.is_deer()) {
        has_deer = true;
      } else if (c.is_boar()) {
        has_boar = true;
      } else if (c.is_butterfly()) {
        has_butterfly = true;
      } else if (c.is_bird()) {
        num_birds++;
      }
    } else {
      num_cards++;
      if (mode == GameMode::Korean && c.is_double_chaff()) {
        num_cards++;
      }
    }
  }

  // Check for light cards
  if (num_lights >= 5) {
    p.result.five_lights = 1;
  } else if (num_lights >= 4) {
    if (has_rain) {
      p.result.rain_four_lights = 1;
    } else {
      p.result.four_lights = 1;
    }
  } else if (num_lights >= 3) {
    if (!has_rain) {
      p.result.three_lights = 1;
    } else if (mode == GameMode::Korean) {
      p.result.rain_three_lights = 1;
    }
  }

  // Check for red & blue ribbons
  if (num_red >= 3) p.result.red_ribbons = 1;
  if (num_blue >= 3) p.result.blue_ribbons = 1;

  if (mode != GameMode::Korean) {
    if (has_boar && has_deer && has_butterfly) {
      p.result.boar_deer_butterfly = 1;
    }
    if (has_sakecup && mode != GameMode::Bet) {
      if (has_moon) p.result.moon_meets_sakecup = 1;
      if (has_flower) p.result.flower_meets_sakecup = 1;
    }
  } else {
    if (num_birds >= 3) p.result.five_birds = 1;
    if (num_grass >= 3) p.result.normal_ribbons = 1;
  }

  if (num_cards >= 10) {
    p.result.cards = num_cards - 9;
  }

  if (num_animals >= 5) {
    p.result.animals = num_animals - 4;
  }

  if (mode == GameMode::Korean && has_sakecup) {
    // In Korean game Sake Cup cannot count as both animal and chaff
    if (p.result.cards - 2 > p.result.animals) {
      p.result.animals = std::max(0, p.result.animals - 1);
    } else {
      p.result.cards = std::max(0, p.result.cards - 2);
    }
  }

  if (num_ribbons >= 5) {
    p.result.ribbons = num_ribbons - 4;
  }

  // Calculate total score
  if (mode == GameMode::Korean) {
    p.result.score += p.result.five_lights * 15;
    p.result.score += p.result.four_lights * 4;
    p.result.score += p.result.rain_four_lights * 4;
    p.result.score += p.result.three_lights * 3;
    p.result.score += p.result.rain_three_lights * 2;

    p.result.score += p.result.red_ribbons * 3;
    p.result.score += p.result.blue_ribbons * 3;
    p.result.score += p.result.normal_ribbons * 3;
    p.result.score += p.result.ribbons;

    p.result.score += p.result.five_birds * 5;
    p.result.score += p.result.animals;

    p.result.score += p.result.cards;
  } else {
    p.result.score += p.result.five_lights * 15;
    p.result.score += p.result.four_lights * 10;
    p.result.score += p.result.rain_four_lights * 8;
    p.result.score += p.result.three_lights * 6;

    p.result.score += p.result.red_ribbons * 6;
    p.result.score += p.result.blue_ribbons * 6;
    p.result.score += p.result.ribbons;

    p.result.score += p.result.boar_deer_butterfly * 5;
    p.result.score += p.result.animals;

    p.result.score += p.result.flower_meets_sakecup * 3;
    p.result.score += p.result.moon_meets_sakecup * 3;

    p.result.score += p.result.cards;

    if (p.result.score == 0 && p.hand.empty() && opn.hand.empty()) {
      if (dealer == player_idx && opn.result.score == 0) {
        p.result.dealer = 1;
        p.result.score = 6;
      }
    }
  }
}

void GameState::calc_add_result(int player_idx) {
  auto& p = players[player_idx];
  const auto& opn = players[1 - player_idx];

  p.result.score += p.num_continue;

  if (p.num_continue >= 4) {
    p.result.score *= 4;
  } else if (p.num_continue == 3) {
    p.result.score *= 2;
  }

  if (p.result.animals >= 3) {
    p.result.score *= 2;
  }

  if (p.result.five_lights || p.result.four_lights ||
      p.result.rain_four_lights || p.result.three_lights ||
      p.result.rain_three_lights) {
    bool opn_has_light = false;
    for (const auto& c : opn.captured) {
      if (c.type() == CardType::Light) {
        opn_has_light = true;
        break;
      }
    }
    if (!opn_has_light) {
      p.result.score *= 2;
    }
  }

  if (p.result.cards > 0) {
    int opn_num_cards = opn.count_chaff(GameMode::Korean);
    if (opn_num_cards <= 5) {
      p.result.score *= 2;
    }
  }
}

std::vector<YakuItem> GameState::get_yaku_items(int player_idx,
                                                bool diff_only) const {
  const auto& p = players[player_idx];
  YakuResult cur = p.result;

  if (diff_only) {
    cur.five_lights -= p.prev_result.five_lights;
    cur.rain_four_lights -= p.prev_result.rain_four_lights;
    cur.four_lights -= p.prev_result.four_lights;
    cur.three_lights -= p.prev_result.three_lights;
    cur.rain_three_lights -= p.prev_result.rain_three_lights;
    cur.red_ribbons -= p.prev_result.red_ribbons;
    cur.blue_ribbons -= p.prev_result.blue_ribbons;
    cur.normal_ribbons -= p.prev_result.normal_ribbons;
    cur.boar_deer_butterfly -= p.prev_result.boar_deer_butterfly;
    cur.five_birds -= p.prev_result.five_birds;
    cur.flower_meets_sakecup -= p.prev_result.flower_meets_sakecup;
    cur.moon_meets_sakecup -= p.prev_result.moon_meets_sakecup;
    cur.dealer -= p.prev_result.dealer;

    cur.ribbons = (p.prev_result.ribbons == cur.ribbons) ? 0 : cur.ribbons;
    cur.animals = (p.prev_result.animals == cur.animals) ? 0 : cur.animals;
    cur.cards = (p.prev_result.cards == cur.cards) ? 0 : cur.cards;
  }

  const std::vector<std::uint8_t> r_lights = {0, 8, 28, 40, 44};
  const std::vector<std::uint8_t> r_red_ribbons = {1, 5, 9};
  const std::vector<std::uint8_t> r_blue_ribbons = {21, 33, 37};
  const std::vector<std::uint8_t> r_normal_ribbons = {13, 17, 25};
  const std::vector<std::uint8_t> r_ribbons = {13, 17, 25, 42, 1,
                                               5,  9,  21, 33, 37};
  const std::vector<std::uint8_t> r_bdb = {20, 24, 36};
  const std::vector<std::uint8_t> r_birds = {4, 12, 29};
  const std::vector<std::uint8_t> r_animals = {4,  12, 16, 20, 24,
                                               29, 32, 36, 41};
  const std::vector<std::uint8_t> r_fms = {32, 8};
  const std::vector<std::uint8_t> r_mms = {32, 28};
  const std::vector<std::uint8_t> r_cards = {
      2,  3,  6,  7,  10, 11, 14, 15, 18, 19, 22, 23, 26,
      27, 30, 31, 32, 34, 35, 38, 39, 43, 45, 46, 47};
  const std::vector<std::uint8_t> r_empty = {};

  std::vector<YakuItem> items;
  auto add_if = [&](int flag_val, int total_val, const char* key, int mult,
                    const std::vector<std::uint8_t>& ids) {
    if (flag_val > 0) {
      items.push_back(
          YakuItem{key, msg(key), total_val * mult, ids});
    }
  };

  if (mode == GameMode::Korean) {
    add_if(cur.five_lights, p.result.five_lights, "five_lights", 15, r_lights);
    add_if(cur.four_lights, p.result.four_lights, "four_lights", 4, r_lights);
    add_if(cur.rain_four_lights, p.result.rain_four_lights, "rain_four_lights",
           4, r_lights);
    add_if(cur.three_lights, p.result.three_lights, "three_lights", 3,
           r_lights);
    add_if(cur.rain_three_lights, p.result.rain_three_lights,
           "rain_three_lights", 2, r_lights);
    add_if(cur.red_ribbons, p.result.red_ribbons, "red_ribbons", 3,
           r_red_ribbons);
    add_if(cur.blue_ribbons, p.result.blue_ribbons, "blue_ribbons", 3,
           r_blue_ribbons);
    add_if(cur.normal_ribbons, p.result.normal_ribbons, "normal_ribbons", 3,
           r_normal_ribbons);
    add_if(cur.ribbons, p.result.ribbons, "ribbons", 1, r_ribbons);
    add_if(cur.five_birds, p.result.five_birds, "five_birds", 5, r_birds);
    add_if(cur.animals, p.result.animals, "animals", 1, r_animals);
    add_if(cur.cards, p.result.cards, "cards", 1, r_cards);
  } else {
    add_if(cur.five_lights, p.result.five_lights, "five_lights", 15, r_lights);
    add_if(cur.four_lights, p.result.four_lights, "four_lights", 10, r_lights);
    add_if(cur.rain_four_lights, p.result.rain_four_lights, "rain_four_lights",
           8, r_lights);
    add_if(cur.three_lights, p.result.three_lights, "three_lights", 6,
           r_lights);
    add_if(cur.red_ribbons, p.result.red_ribbons, "red_ribbons", 6,
           r_red_ribbons);
    add_if(cur.blue_ribbons, p.result.blue_ribbons, "blue_ribbons", 6,
           r_blue_ribbons);
    add_if(cur.ribbons, p.result.ribbons, "ribbons", 1, r_ribbons);
    add_if(cur.boar_deer_butterfly, p.result.boar_deer_butterfly,
           "boar_deer_butterfly", 5, r_bdb);
    add_if(cur.animals, p.result.animals, "animals", 1, r_animals);
    add_if(cur.flower_meets_sakecup, p.result.flower_meets_sakecup,
           "flower_meets_sakecup", 3, r_fms);
    add_if(cur.moon_meets_sakecup, p.result.moon_meets_sakecup,
           "moon_meets_sakecup", 3, r_mms);
    add_if(cur.cards, p.result.cards, "cards", 1, r_cards);
    add_if(cur.dealer, p.result.dealer, "dealer", 6, r_empty);
  }

  return items;
}

void GameState::highlight_yaku_cards(int player_idx,
                                     const std::vector<std::uint8_t>& ids) {
  clear_card_effects();
  for (auto& c : players[player_idx].captured) {
    if (std::find(ids.begin(), ids.end(), c.id()) != ids.end()) {
      c.render_effect |= static_cast<unsigned int>(CardEffect::Box);
    }
  }
}

void GameState::clear_card_effects() {
  for (auto& c : desk_cards) c.render_effect = 0;
  for (int p = 0; p < 2; ++p) {
    for (auto& c : players[p].hand) c.render_effect = 0;
    for (auto& c : players[p].captured) c.render_effect = 0;
  }
}

// ============================================================================
// Bot AI Implementation (ported 1:1 from SDLHana CBot)
// ============================================================================

int GameState::bot_select_hand_card() {
  std::array<int, 12> hand_pct{};
  std::array<int, 12> opn_pct{};
  std::vector<BotMove> moves;

  bot_analyze_hand(hand_pct, opn_pct);
  bot_analyze_moves(moves, hand_pct, opn_pct);

  if (moves.empty()) {
    bot_wanted_move_month_ = -1;
    bot_wanted_move_desk_ = -1;
    return bot_discard_card(opn_pct);
  }

  int index = -1;
  int handtype = HAND_MAXNUM + 1;
  int prev = -1;

  for (int i = 0; i < static_cast<int>(moves.size()); ++i) {
    for (int j = handtype; j < HAND_COUNT; ++j) {
      if (moves[i].hand[j] > hand_pct[j] && moves[i].hand[j] > prev) {
        handtype = j;
        prev = moves[i].hand[j];
        index = i;
      }
    }
  }

  if (index != -1 && prev > 100) {
    bot_wanted_move_month_ = moves[index].month;
    bot_wanted_move_desk_ = moves[index].desk_index;
    return moves[index].hand_index;
  }

  int goal = bot_analyze_goal(moves, hand_pct);
  index = 0;
  handtype = HAND_MAXNUM + 1;
  prev = -999999;

  const auto& bot_hand = players[1].hand;

  for (int i = 0; i < static_cast<int>(moves.size()); ++i) {
    for (int j = handtype; j < HAND_COUNT; ++j) {
      int sc = 0;
      if (j == goal || goal == -1) {
        sc += 80;
      }
      sc += static_cast<int>(desk_cards[moves[i].desk_index].type()) * 15;
      sc += static_cast<int>(bot_hand[moves[i].hand_index].type()) * 10;
      sc += moves[i].hand[j] - hand_pct[j];
      if (opn_pct[HAND_MAX] >= 60 || opn_pct[HAND_SAKECUP] > 0) {
        sc += (opn_pct[HAND_MAX] - moves[i].opnhand[HAND_MAX]) * 2;
        sc += bot_card_is_dangerous(desk_cards[moves[i].desk_index], opn_pct) *
              opn_pct[HAND_MAX] / 2;
      }
      for (int k = HAND_MAXNUM + 1; k < HAND_COUNT; ++k) {
        sc += (moves[i].hand[k] - hand_pct[k]) / 5;
      }
      if (sc > prev) {
        prev = sc;
        index = i;
      }
    }
  }

  bot_wanted_move_month_ = moves[index].month;
  bot_wanted_move_desk_ = moves[index].desk_index;
  return moves[index].hand_index;
}

int GameState::bot_discard_card(const std::array<int, 12>& opn_pct) const {
  const auto& bot_hand = players[1].hand;
  int index = 0;
  int max_sc = -999999;

  for (int i = 0; i < static_cast<int>(bot_hand.size()); ++i) {
    Card c = bot_hand[i];
    int sc = 5000 - static_cast<int>(c.type()) * 30;
    sc += (bot_card_is_safe(c) ? 300 : 0);
    sc -= bot_card_is_dangerous(c, opn_pct) * 150;
    sc += bot_num_month_in_hand(c.month()) * 50;
    sc += bot_num_month_captured(c.month()) * 60;

    if (sc > max_sc) {
      index = i;
      max_sc = sc;
    }
  }
  return index;
}

bool GameState::bot_want_to_continue() {
  if (mode == GameMode::Bet) {
    return false;
  }

  std::array<int, 12> hand_pct{};
  std::array<int, 12> opn_pct{};
  std::vector<BotMove> moves;

  bot_analyze_hand(hand_pct, opn_pct);
  bot_analyze_moves(moves, hand_pct, opn_pct);

  int sc = 50;
  for (int i = HAND_MAXNUM + 1; i < HAND_COUNT; ++i) {
    sc += (hand_pct[i] - opn_pct[i]) * i / 80;
  }

  for (const auto& mv : moves) {
    for (int j = HAND_MAXNUM + 1; j < HAND_COUNT; ++j) {
      if (mv.hand[j] > hand_pct[j] && mv.hand[j] >= 100) {
        sc += (mv.hand[j] - hand_pct[j]) * (j - HAND_MAXNUM) / 2;
      }
      sc += (mv.hand[j] - hand_pct[j]) * (j - HAND_MAXNUM) / 15;
      sc += (opn_pct[j] - mv.opnhand[j]) * (j - HAND_MAXNUM) / 30;
    }
  }

  if (mode != GameMode::Korean) {
    std::array<int, 12> sim_hand{};
    std::array<int, 12> sim_opn{};
    auto saved_captured = players[1].captured;
    for (const auto& c : players[1].hand) {
      players[1].captured.push_back(c);
    }
    for (int i = 0; i < num_desk_cards; ++i) {
      if (desk_cards[i].is_valid()) {
        players[1].captured.push_back(desk_cards[i]);
      }
    }
    bot_analyze_hand(sim_hand, sim_opn);
    players[1].captured = std::move(saved_captured);
    for (int j = HAND_MAXNUM + 1; j < HAND_COUNT; ++j) {
      sc += (sim_hand[j] - hand_pct[j]) * (j - HAND_MAXNUM) / 25;
    }
  }

  for (const auto& c : players[1].hand) {
    if (bot_card_is_safe(c)) {
      sc += 10;
    } else {
      sc -= bot_card_is_dangerous(c, opn_pct) * 2;
    }
  }

  for (int i = 0; i < num_desk_cards; ++i) {
    if (!desk_cards[i].is_valid()) continue;
    if (bot_card_is_safe(desk_cards[i])) {
      sc += 5;
    } else {
      sc -= (bot_card_is_dangerous(desk_cards[i], opn_pct) - 1) * 8;
    }
  }

  return (random_int(1, 300) < sc);
}

int GameState::bot_select_desk_card(int month, const Card& drawn) {
  auto matches = matching_desk_slots(month);
  if (matches.empty()) return 0;
  if (matches.size() == 1) return matches[0];

  if (month == bot_wanted_move_month_ &&
      (bot_wanted_move_desk_ == matches[0] ||
       bot_wanted_move_desk_ == matches[1])) {
    return bot_wanted_move_desk_;
  }

  std::array<int, 12> hand_pct{};
  std::array<int, 12> opn_pct{};
  std::vector<BotMove> moves;
  bot_analyze_hand(hand_pct, opn_pct);
  bot_analyze_moves(moves, hand_pct, opn_pct);

  std::array<std::array<int, 12>, 2> hand_sim{};
  std::array<std::array<int, 12>, 2> opn_sim{};

  players[1].captured.push_back(drawn);
  players[1].captured.push_back(desk_cards[matches[0]]);
  bot_analyze_hand(hand_sim[0], opn_sim[0]);
  players[1].captured.back() = desk_cards[matches[1]];
  bot_analyze_hand(hand_sim[1], opn_sim[1]);
  players[1].captured.pop_back();
  players[1].captured.pop_back();

  int goal = bot_analyze_goal(moves, hand_pct);
  int chosen = 0;
  int max_sc = -999999;

  for (int i = 0; i < 2; ++i) {
    for (int j = HAND_MAXNUM + 1; j < HAND_COUNT; ++j) {
      int sc = 0;
      if (j == goal || goal == -1) sc += 80;
      sc += static_cast<int>(desk_cards[matches[i]].type()) * 15;
      sc += hand_sim[i][j] - hand_pct[j];
      if (hand_sim[i][j] - hand_pct[j] > 0 && hand_sim[i][j] >= 100) {
        sc += 500;
      }
      if (opn_pct[HAND_MAX] >= 60 || opn_pct[HAND_SAKECUP] > 0) {
        sc += (opn_pct[HAND_MAX] - opn_sim[i][HAND_MAX]) * 2;
        sc += bot_card_is_dangerous(desk_cards[matches[i]], opn_pct) *
              opn_pct[HAND_MAX] / 2;
      }
      for (int k = HAND_MAXNUM + 1; k < HAND_COUNT; ++k) {
        sc += (hand_sim[i][k] - hand_pct[k]) / 5;
      }
      if (sc > max_sc) {
        max_sc = sc;
        chosen = i;
      }
    }
  }

  return matches[chosen];
}

void GameState::bot_analyze_moves(std::vector<BotMove>& moves,
                                  const std::array<int, 12>& /*base_hand*/,
                                  const std::array<int, 12>& /*base_opn*/) {
  moves.clear();
  for (int i = 0; i < num_desk_cards; ++i) {
    if (!desk_cards[i].is_valid()) continue;
    for (int j = 0; j < static_cast<int>(players[1].hand.size()); ++j) {
      if (desk_cards[i] == players[1].hand[j]) {
        BotMove mv{};
        mv.desk_index = i;
        mv.hand_index = j;
        mv.month = players[1].hand[j].month();

        players[1].captured.push_back(players[1].hand[j]);
        players[1].captured.push_back(desk_cards[i]);
        bot_analyze_hand(mv.hand, mv.opnhand);
        players[1].captured.pop_back();
        players[1].captured.pop_back();

        moves.push_back(mv);
      }
    }
  }
}

void GameState::bot_analyze_hand(std::array<int, 12>& hand_pct,
                                 std::array<int, 12>& opn_pct) const {
  auto eval_player = [&](const PlayerState& ps, std::array<int, 12>& out) {
    int num_lights = 0, num_animals = 0, num_ribbons = 0;
    int num_red = 0, num_blue = 0, num_grass = 0;
    int num_cards = 0, num_birds = 0, num_boar = 0;
    bool has_rain = false, has_sakecup = false, has_moon = false,
         has_flower = false;

    for (const auto& c : ps.captured) {
      auto type = c.type();
      if (type == CardType::Light) {
        num_lights++;
        if (c.is_rain())
          has_rain = true;
        else if (c.is_moon())
          has_moon = true;
        else if (c.is_flower())
          has_flower = true;
      } else if (type == CardType::RibbonRed) {
        num_red++;
        num_ribbons++;
      } else if (type == CardType::RibbonBlue) {
        num_blue++;
        num_ribbons++;
      } else if (type == CardType::Ribbon) {
        num_ribbons++;
        if (c.month() != 11) num_grass++;
      } else if (type == CardType::Animal) {
        num_animals++;
        if (c.is_sake_cup()) {
          num_cards += (mode == GameMode::Korean) ? 2 : 1;
          has_sakecup = true;
        } else if (c.is_deer() || c.is_boar() || c.is_butterfly()) {
          num_boar++;
        } else if (c.is_bird()) {
          num_birds++;
        }
      } else {
        num_cards++;
        if (mode == GameMode::Korean && c.is_double_chaff()) {
          num_cards++;
        }
      }
    }

    out[HAND_CARDS] = num_cards * 10;
    out[HAND_ANIMALS] = num_animals * 20;
    out[HAND_RIBBONS] = num_ribbons * 20;
    out[HAND_LIGHTS] = num_lights * 34 - (has_rain ? 17 : 0);
    out[HAND_RED_RIBBONS] = num_red * 34;
    out[HAND_BLUE_RIBBONS] = num_blue * 34;

    if (mode == GameMode::Korean) {
      out[HAND_NORMAL_RIBBONS] = num_grass * 34;
      out[HAND_BIRD] = num_birds * 34;
      out[HAND_BOAR] = -1;
      out[HAND_SAKECUP] = -1;
    } else {
      out[HAND_BOAR] = num_boar * 34;
      if (mode != GameMode::Bet) {
        out[HAND_SAKECUP] = (has_sakecup ? 60 : 0) + (has_moon ? 40 : 0) +
                            (has_flower ? 40 : 0);
      } else {
        out[HAND_SAKECUP] = -1;
      }
      out[HAND_NORMAL_RIBBONS] = -1;
      out[HAND_BIRD] = -1;
    }
  };

  eval_player(players[1], hand_pct);
  eval_player(players[0], opn_pct);

  if (hand_pct[HAND_RED_RIBBONS] > 0 && opn_pct[HAND_RED_RIBBONS] > 0) {
    hand_pct[HAND_RED_RIBBONS] = 0;
    opn_pct[HAND_RED_RIBBONS] = 0;
  }
  if (hand_pct[HAND_BLUE_RIBBONS] > 0 && opn_pct[HAND_BLUE_RIBBONS] > 0) {
    hand_pct[HAND_BLUE_RIBBONS] = 0;
    opn_pct[HAND_BLUE_RIBBONS] = 0;
  }
  if (hand_pct[HAND_NORMAL_RIBBONS] > 0 && opn_pct[HAND_NORMAL_RIBBONS] > 0) {
    hand_pct[HAND_NORMAL_RIBBONS] = 0;
    opn_pct[HAND_NORMAL_RIBBONS] = 0;
  }
  if (hand_pct[HAND_BOAR] > 0 && opn_pct[HAND_BOAR] > 0) {
    hand_pct[HAND_BOAR] = 0;
    opn_pct[HAND_BOAR] = 0;
  }
  if (hand_pct[HAND_BIRD] > 0 && opn_pct[HAND_BIRD] > 0) {
    hand_pct[HAND_BIRD] = 0;
    opn_pct[HAND_BIRD] = 0;
  }
  if (hand_pct[HAND_LIGHTS] >= 60) opn_pct[HAND_LIGHTS] = 0;
  if (opn_pct[HAND_LIGHTS] >= 60) hand_pct[HAND_LIGHTS] = 0;
  if (hand_pct[HAND_SAKECUP] >= 60) opn_pct[HAND_SAKECUP] = 0;
  if (opn_pct[HAND_SAKECUP] >= 60) hand_pct[HAND_SAKECUP] = 0;

  opn_pct[HAND_MAX] = 0;
  opn_pct[HAND_MAXNUM] = -1;
  for (int i = HAND_MAXNUM + 1; i < HAND_COUNT; ++i) {
    if (opn_pct[i] >= opn_pct[HAND_MAX]) {
      opn_pct[HAND_MAX] = opn_pct[i];
      opn_pct[HAND_MAXNUM] = i;
    }
  }

  hand_pct[HAND_MAX] = 0;
  hand_pct[HAND_MAXNUM] = -1;
  for (int i = HAND_MAXNUM + 1; i < HAND_COUNT; ++i) {
    if (hand_pct[i] >= hand_pct[HAND_MAX]) {
      hand_pct[HAND_MAX] = hand_pct[i];
      hand_pct[HAND_MAXNUM] = i;
    }
  }
}

int GameState::bot_analyze_goal(const std::vector<BotMove>& moves,
                                const std::array<int, 12>& hand_pct) const {
  std::array<int, 12> goalvalue{};
  for (const auto& mv : moves) {
    for (int j = HAND_MAX + 1; j < HAND_COUNT; ++j) {
      if (mv.hand[j] > hand_pct[j]) {
        goalvalue[j] += mv.hand[j] * (j - HAND_MAX);
      }
    }
  }
  for (const auto& mv : moves) {
    int m = mv.hand[HAND_MAXNUM];
    if (m >= 0 && m < HAND_COUNT && goalvalue[m] > 0) {
      goalvalue[m] += mv.hand[HAND_MAX] * (m - HAND_MAX) / 2;
    }
  }

  auto boost_for_card = [&](const Card& c, int factor) {
    if (!c.is_valid()) return;
    if (c.type() == CardType::Light) {
      if (hand_pct[HAND_LIGHTS] >= 0 && goalvalue[HAND_LIGHTS] > 0) {
        goalvalue[HAND_LIGHTS] += 10 * factor;
      }
      if ((c.is_moon() || c.is_flower()) && hand_pct[HAND_SAKECUP] >= 0 &&
          goalvalue[HAND_SAKECUP] > 0) {
        goalvalue[HAND_SAKECUP] += 15 * factor;
      }
    } else if (c.type() == CardType::Animal) {
      if (goalvalue[HAND_ANIMALS] > 0) goalvalue[HAND_ANIMALS] += 5 * factor;
      if (c.is_bird() && hand_pct[HAND_BIRD] >= 0 && goalvalue[HAND_BIRD] > 0) {
        goalvalue[HAND_BIRD] += (factor == 2 ? 15 : 8);
      }
      if ((c.is_boar() || c.is_deer() || c.is_butterfly()) &&
          hand_pct[HAND_BOAR] >= 0 && goalvalue[HAND_BOAR] > 0) {
        goalvalue[HAND_BOAR] += (factor == 2 ? 15 : 8);
      }
    } else if (c.type() == CardType::RibbonRed) {
      if (hand_pct[HAND_RED_RIBBONS] >= 0 && goalvalue[HAND_RED_RIBBONS] > 0) {
        goalvalue[HAND_RED_RIBBONS] += (factor == 2 ? 15 : 8);
      }
      if (goalvalue[HAND_RIBBONS] > 0) goalvalue[HAND_RIBBONS] += 5 * factor;
    } else if (c.type() == CardType::RibbonBlue) {
      if (hand_pct[HAND_BLUE_RIBBONS] >= 0 &&
          goalvalue[HAND_BLUE_RIBBONS] > 0) {
        goalvalue[HAND_BLUE_RIBBONS] += (factor == 2 ? 15 : 8);
      }
      if (goalvalue[HAND_RIBBONS] > 0) goalvalue[HAND_RIBBONS] += 5 * factor;
    } else if (c.type() == CardType::Ribbon) {
      if (c.month() != 11 && hand_pct[HAND_NORMAL_RIBBONS] >= 0 &&
          goalvalue[HAND_NORMAL_RIBBONS] > 0) {
        goalvalue[HAND_NORMAL_RIBBONS] += (factor == 2 ? 15 : 8);
      }
      if (goalvalue[HAND_RIBBONS] > 0) goalvalue[HAND_RIBBONS] += 5 * factor;
    } else {
      if (goalvalue[HAND_CARDS] > 0) goalvalue[HAND_CARDS] += 1;
    }
  };

  for (const auto& c : players[1].hand) boost_for_card(c, 2);
  for (int i = 0; i < num_desk_cards; ++i) boost_for_card(desk_cards[i], 1);

  int max_val = 0;
  int max_idx = -1;
  for (int i = 0; i < HAND_COUNT; ++i) {
    if (goalvalue[i] > max_val) {
      max_val = goalvalue[i];
      max_idx = i;
    }
  }
  return max_idx;
}

int GameState::bot_num_month_in_hand(int month) const {
  int count = 0;
  for (const auto& c : players[1].hand) {
    if (c.month() == month) ++count;
  }
  return count;
}

int GameState::bot_num_month_exposed(int month) const {
  int count = bot_num_month_captured(month);
  for (int i = 0; i < num_desk_cards; ++i) {
    if (desk_cards[i].is_valid() && desk_cards[i].month() == month) {
      ++count;
    }
  }
  return count;
}

int GameState::bot_num_month_captured(int month) const {
  int count = 0;
  for (const auto& c : players[1].captured) {
    if (c.month() == month) ++count;
  }
  for (const auto& c : players[0].captured) {
    if (c.month() == month) ++count;
  }
  return count;
}

int GameState::bot_num_month_invisible(int month) const {
  return 4 - bot_num_month_exposed(month) - bot_num_month_in_hand(month);
}

bool GameState::bot_card_is_safe(const Card& c) const {
  return bot_num_month_invisible(c.month()) <= 0;
}

int GameState::bot_card_is_dangerous(
    const Card& c, const std::array<int, 12>& opn_pct) const {
  if (bot_card_is_safe(c)) return 0;

  auto either_captured = [&](std::uint8_t id) {
    return players[1].has_captured(id) || players[0].has_captured(id);
  };

  for (int i = HAND_COUNT - 1; i > HAND_MAXNUM; --i) {
    if (opn_pct[i] >= ((i == HAND_SAKECUP) ? 30 : 60)) {
      switch (i) {
        case HAND_CARDS:
          return HAND_CARDS;
        case HAND_RIBBONS: {
          if (c.type() == CardType::Ribbon || c.type() == CardType::RibbonRed ||
              c.type() == CardType::RibbonBlue) {
            return HAND_RIBBONS;
          }
          int j = c.month() - 1;
          if (j == 7 || j == 11) break;
          int k = (j == 10) ? 2 : 1;
          if (either_captured(static_cast<std::uint8_t>(j * 4 + k))) break;
          return HAND_RIBBONS;
        }
        case HAND_ANIMALS: {
          if (c.type() == CardType::Animal) return HAND_ANIMALS;
          int j = c.month() - 1;
          if (j == 0 || j == 2 || j == 11) break;
          int k = (j == 10 || j == 7) ? 1 : 0;
          if (either_captured(static_cast<std::uint8_t>(j * 4 + k))) break;
          return HAND_ANIMALS;
        }
        case HAND_SAKECUP:
          if ((c.month() == 3 && !either_captured(8)) ||
              (c.month() == 8 && !either_captured(28)) ||
              (c.month() == 9 && !either_captured(32))) {
            return HAND_SAKECUP;
          }
          break;
        case HAND_RED_RIBBONS:
          if ((c.month() == 1 && !either_captured(1)) ||
              (c.month() == 2 && !either_captured(5)) ||
              (c.month() == 3 && !either_captured(9))) {
            return HAND_RED_RIBBONS;
          }
          break;
        case HAND_BLUE_RIBBONS:
          if ((c.month() == 6 && !either_captured(21)) ||
              (c.month() == 9 && !either_captured(33)) ||
              (c.month() == 10 && !either_captured(37))) {
            return HAND_BLUE_RIBBONS;
          }
          break;
        case HAND_NORMAL_RIBBONS:
          if ((c.month() == 4 && !either_captured(13)) ||
              (c.month() == 5 && !either_captured(17)) ||
              (c.month() == 7 && !either_captured(25))) {
            return HAND_NORMAL_RIBBONS;
          }
          break;
        case HAND_BOAR:
          if ((c.month() == 6 && !either_captured(20)) ||
              (c.month() == 7 && !either_captured(24)) ||
              (c.month() == 10 && !either_captured(36))) {
            return HAND_BOAR;
          }
          break;
        case HAND_BIRD:
          if ((c.month() == 2 && !either_captured(4)) ||
              (c.month() == 4 && !either_captured(12)) ||
              (c.month() == 8 && !either_captured(29))) {
            return HAND_BIRD;
          }
          break;
        case HAND_LIGHTS:
          if ((c.month() == 1 && !either_captured(0)) ||
              (c.month() == 3 && !either_captured(8)) ||
              (c.month() == 8 && !either_captured(28)) ||
              (c.month() == 11 && !either_captured(40)) ||
              (c.month() == 12 && !either_captured(44))) {
            return HAND_LIGHTS;
          }
          break;
      }
    }
  }
  return 0;
}
