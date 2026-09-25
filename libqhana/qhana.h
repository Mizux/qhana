#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

inline constexpr int MAX_HISTORY_ROUNDS = 30;

enum class CardType : std::uint8_t {
  None = 0,
  Animal,
  Ribbon,
  RibbonRed,
  RibbonBlue,
  Light,
};

enum class CardEffect : std::uint8_t {
  None = 0,
  Dark = (1 << 0),
  Box = (1 << 1),
};

enum class GameMode : int {
  KoiKoi = 0,
  Bet = 1,
  Korean = 2,
};

enum class Language : int {
  English = 0,
  French = 1,
  Japanese = 2,
};

enum class AnimSpeed : int {
  VerySlow = 0,
  Slow = 1,
  Middle = 2,
  Fast = 3,
  VeryFast = 4,
};

class Card {
 public:
  using id_type = std::uint8_t;

  constexpr Card(id_type value = 255) : render_effect(0), id_(value) {}

  constexpr id_type id() const { return id_; }
  constexpr id_type month() const { return id_ / 4 + 1; }
  constexpr int sub_index() const { return id_ & 3; }

  // In Hanafuda / SDLHana, two cards match if they belong to the same month
  constexpr bool operator==(const Card& c) const {
    return is_valid() && c.is_valid() && month() == c.month();
  }
  constexpr bool operator!=(const Card& c) const { return !(*this == c); }

  constexpr bool same_id(const Card& c) const { return id_ == c.id_; }
  constexpr bool same_month(const Card& c) const {
    return is_valid() && c.is_valid() && month() == c.month();
  }

  CardType type() const;

  inline int order() const {
    switch (type()) {
      case CardType::Light:
        return 5;
      case CardType::Animal:
        return 4;
      case CardType::RibbonBlue:
        return 3;
      case CardType::RibbonRed:
        return 2;
      case CardType::Ribbon:
        return 1;
      case CardType::None:
        return 0;
    }
    return 0;
  }

  constexpr bool is_rain() const { return id_ == 40; }
  constexpr bool is_sake_cup() const { return id_ == 32; }
  constexpr bool is_boar() const { return id_ == 24; }
  constexpr bool is_deer() const { return id_ == 36; }
  constexpr bool is_butterfly() const { return id_ == 20; }
  constexpr bool is_moon() const { return id_ == 28; }
  constexpr bool is_flower() const { return id_ == 8; }
  constexpr bool is_bird() const { return id_ == 4 || id_ == 12 || id_ == 29; }
  constexpr bool is_double_chaff() const { return id_ == 43 || id_ == 45; }

  constexpr bool is_valid() const { return id_ < 48; }
  constexpr void destroy() { id_ = 255; }

  unsigned int render_effect = 0;

 private:
  id_type id_;
};

struct YakuResult {
  int score = 0;

  int five_lights = 0;
  int four_lights = 0;
  int rain_four_lights = 0;
  int three_lights = 0;
  int rain_three_lights = 0;  // Korean mode

  int red_ribbons = 0;
  int blue_ribbons = 0;
  int normal_ribbons = 0;  // Korean mode
  int ribbons = 0;

  int boar_deer_butterfly = 0;  // Japanese mode
  int animals = 0;
  int five_birds = 0;  // Korean mode

  int flower_meets_sakecup = 0;  // Japanese mode
  int moon_meets_sakecup = 0;    // Japanese mode

  int cards = 0;
  int dealer = 0;
};

struct YakuItem {
  std::string key;
  std::string name;
  int points = 0;
  std::vector<std::uint8_t> card_ids;
};

struct PlayerState {
  bool is_bot = false;
  std::vector<Card> hand;
  std::vector<Card> captured;

  YakuResult result{};
  YakuResult prev_result{};

  int num_continue = 0;
  int num_leave_three = 0;

  void reset();
  bool has_captured(std::uint8_t card_id) const;
  std::vector<Card> captured_special() const;
  std::vector<Card> captured_normal() const;

  int count_lights() const;
  int count_animals() const;
  int count_ribbons() const;
  int count_chaff(GameMode mode) const;
};

enum class GamePhase {
  SelectHandCard,
  SelectDeskCardForHand,
  SelectDeskCardForDrawn,
  AskKoiKoi,
  AskDoubleUp,
  AskBigOrSmall,
  RoundOver,
};

std::string month_name(int month);
std::string month_flower_name(int month);
std::string card_name(const Card& c);
std::string card_short_label(const Card& c);
std::string card_type_name(CardType type);
std::string card_type_badge(CardType type);
std::string game_mode_name(GameMode mode, Language lang = Language::English);
std::string language_name(Language lang, Language display_lang = Language::English);
std::string anim_speed_name(AnimSpeed speed, Language lang = Language::English);
int anim_speed_ms(AnimSpeed speed);
std::string tr_msg(std::string_view key, Language lang = Language::English);

class GameState {
 public:
  GameState();

  void new_game();
  void new_round();

  int max_hand_cards() const { return (mode == GameMode::Bet) ? 6 : 8; }
  int remaining_deck_count() const;

  // Returns desk slot indices that match the given month
  std::vector<int> matching_desk_slots(int month) const;
  std::vector<Card> valid_desk_cards() const;

  // Player actions
  bool play_hand_card(int hand_idx);
  bool select_desk_card(int desk_slot);
  void answer_koikoi(bool continue_game);
  void answer_double_up(bool want_double);
  void answer_big_or_small(bool is_big);

  // Bot turn execution
  int bot_select_hand_card();
  int bot_select_desk_card(int month, const Card& card);
  bool bot_want_to_continue();
  void step_bot();

  // Scoring & Yaku evaluation
  void calc_result(int player_idx);
  void calc_add_result(int player_idx);
  std::vector<YakuItem> get_yaku_items(int player_idx,
                                       bool diff_only = false) const;

  // Highlight captured cards belonging to newly formed yaku
  void highlight_yaku_cards(int player_idx,
                            const std::vector<std::uint8_t>& ids);
  void clear_card_effects();

  std::string msg(std::string_view key) const { return tr_msg(key, language); }
  void add_log(const std::string& entry);

  // Settings & Persistent state
  GameMode mode = GameMode::KoiKoi;
  Language language = Language::English;
  AnimSpeed anim_speed = AnimSpeed::Fast;
  bool sound_enabled = true;
  bool fullscreen = false;

  // Match state
  int score = 0;
  int round_number = 0;
  int dealer = 0;          // 0 = Human, 1 = Bot
  int current_player = 0;  // 0 = Human, 1 = Bot
  int winner = -1;         // -1 = None/Draw, 0 = Human, 1 = Bot
  GamePhase phase = GamePhase::SelectHandCard;

  std::array<PlayerState, 2> players{};
  std::array<Card, 24> desk_cards{};
  int num_desk_cards = 0;

  // Turn resolution state
  Card last_played_card{255};
  Card last_drawn_card{255};
  Card double_up_card{255};
  bool double_up_won = false;
  int pending_getfour_month = -1;
  bool pending_leavethree = false;
  int pending_old_slot = 999;
  std::array<int, 2> desk_choice_slots{-1, -1};

  std::vector<YakuItem> last_formed_yaku;
  std::vector<YakuItem> round_yaku_summary;
  std::string status_banner;
  std::vector<std::string> game_log;

  // Statistics & History
  std::vector<int> score_history;
  std::vector<int> player_round_scores;
  std::vector<int> bot_round_scores;
  int rounds_won = 0;
  int rounds_lost = 0;
  int rounds_drawn = 0;

 private:
  void reset_deck();
  Card draw_random_card();
  void put_back_to_pile(const Card& c);
  int find_free_desk_slot(int exclude = -1) const;
  void steal_one_chaff_from_opponent(int player_idx);

  void resolve_drawn_card_step();
  void finish_turn_after_captures();
  void finalize_round();

  // Bot AI helpers (ported from SDLHana CBot)
  struct BotMove {
    int hand_index = -1;
    int desk_index = -1;
    std::array<int, 12> hand{};
    std::array<int, 12> opnhand{};
    int month = 0;
  };

  void bot_analyze_moves(std::vector<BotMove>& moves,
                         const std::array<int, 12>& base_hand,
                         const std::array<int, 12>& base_opn);
  void bot_analyze_hand(std::array<int, 12>& hand_pct,
                        std::array<int, 12>& opn_pct) const;
  int bot_analyze_goal(const std::vector<BotMove>& moves,
                       const std::array<int, 12>& hand_pct) const;
  int bot_discard_card(const std::array<int, 12>& opn_pct) const;
  int bot_num_month_in_hand(int month) const;
  int bot_num_month_exposed(int month) const;
  int bot_num_month_captured(int month) const;
  int bot_num_month_invisible(int month) const;
  bool bot_card_is_safe(const Card& c) const;
  int bot_card_is_dangerous(const Card& c,
                            const std::array<int, 12>& opn_pct) const;

  std::array<std::uint8_t, 6> card_flags_{};
  int bot_wanted_move_month_ = -1;
  int bot_wanted_move_desk_ = -1;
};
