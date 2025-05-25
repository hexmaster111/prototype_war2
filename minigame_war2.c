#include <assert.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cplib.h"

/*
 *     War2 States
 *     1 ) Pick card
 *         Once cards are picked
 *     2 ) Compare selcted cards
 *
 */

typedef struct {
    enum CardValue { V_A = 1, V_2, V_3, V_4, V_5, V_6, V_7, V_8, V_9, V_10, V_J, V_Q, V_K } value;
} Card;

const char *CardValueToString(enum CardValue v) {
    switch (v) {
    case V_A:
        return "A";
    case V_2:
        return "2";
    case V_3:
        return "3";
    case V_4:
        return "4";
    case V_5:
        return "5";
    case V_6:
        return "6";
    case V_7:
        return "7";
    case V_8:
        return "8";
    case V_9:
        return "9";
    case V_10:
        return "10";
    case V_J:
        return "Jack";
    case V_Q:
        return "Queen";
    case V_K:
        return "King";
    }
    UNREACHABLE();
    return "";
}

typedef struct {
    int count, cap;
    Card *cards; // WARNING: this pointer changes (realloc)
} CardStack;

CardStack csNew(int init_cap);
void csFree(CardStack *this);
Card csPop(CardStack *this);
void csPush(CardStack *this, Card c);
void csShuffle(CardStack *this);
bool csHasCard(CardStack *this);
void csFill(CardStack *ret);

#define PLAYER_HAND_CARDS (3)

typedef struct {
    CardStack pile;
    CardStack hand;
    Card selected;
    bool has_picked_card;
    Controller controller;
} war_player;

struct war_state {
    war_player players[4];
    enum GameState { PickCard, Compare } state;
    int player_count;
} gs = {0};

bool AllPlayersPickedACard() {
    for (size_t i = 0; i < gs.player_count; i++) {
        if (!gs.players[i].has_picked_card)
            return false;
    }
    return true;
}

#define ARROW_UP (0)
#define ARROW_DOWN (1)
#define ARROW_LEFT (2)
#define ARROW_RIGHT (3)
struct war_assets {
    Texture arrow[4];
} wa = {0};

Card csPopWithin(CardStack *cs, size_t idx) {
    // pops card from idx, shifts cards
    Card ret = cs->cards[idx];
    for (size_t i = idx; i < cs->count - 1; i++) {
        cs->cards[i] = cs->cards[i + 1];
    }
    cs->count -= 1;
    return ret;
}

void DrawPickCards() {
    for (int p = 0; p < gs.player_count; p += 1) {
        war_player it = gs.players[p];
        Rectangle draw_pos = {
            (GetScreenWidth() * .25f) * p,
            GetScreenHeight() * .5f,
            GetScreenWidth() * .25f,
            GetScreenHeight() * .5f,
        };
        //

        DrawRectangleRec(draw_pos, it.has_picked_card ? LIGHTGRAY : DARKGREEN);
        DrawLine(draw_pos.x + draw_pos.width, draw_pos.y, draw_pos.x + draw_pos.width, draw_pos.y + draw_pos.height, WHITE);

        if (it.has_picked_card) {
            // draw Upside down cards
        } else {
            // Draw Up Down Left icons, draw hand
            for (int c = 0; c < (3 > it.hand.count ? it.hand.count : 3); c += 1) {
                float cardwidth = (GetScreenWidth() * .25) * (1.0 / 3.0);
                Vector2 cardpos = {draw_pos.x + c * cardwidth + (48 * .5), draw_pos.y};

                Card crd = it.hand.cards[c];
                DrawText(TextFormat("%d", crd.value), cardpos.x, cardpos.y, 48, YELLOW);

                cardpos.y += 40;
                cardpos.x -= 25;
                DrawTextureEx(wa.arrow[c], cardpos, 0, .25, WHITE);
            }
        }
    }

    DrawText(TextFormat("%d", gs.players[0].pile.count), GetScreenWidth() * .5, 10, 24, BLACK);
}

void UpdatePickCards() {
    for (int p = 0; p < gs.player_count; p += 1) {
        war_player *it = &gs.players[p];
        if (it->has_picked_card)
            continue;

        if (Controller_IsKeyDown(it->controller, Uk_MoveForward)) {
            it->selected = csPopWithin(&it->hand, 0);
            it->has_picked_card = true;
        }

        if (Controller_IsKeyDown(it->controller, Uk_MoveBack)) {
            it->selected = csPopWithin(&it->hand, 1);
            it->has_picked_card = true;
        }

        if (Controller_IsKeyDown(it->controller, Uk_MoveLeft)) {
            it->selected = csPopWithin(&it->hand, 2);
            it->has_picked_card = true;
        }
    }
}

struct CompareState {
    float next_reveal;
    int cards_to_show;
} compare_state = {0};

void UpdateCompare() {
    // reveal players cards
    if (GetTime() > compare_state.next_reveal) {
        compare_state.cards_to_show += 1;
        compare_state.next_reveal = GetTime() + 1.5f;
    }
}

void DrawCompare() {

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight() * .5f, DARKGREEN);

    for (size_t i = 0; i < compare_state.cards_to_show; i++) {
        float cardwidth = (GetScreenWidth() * (1.0 / 3.0));
        war_player it = gs.players[i];
        DrawText(CardValueToString(it.selected.value), cardwidth * i, GetScreenHeight() * .25, 42, YELLOW);
    }
}

void War2NewGame() {
    for (int i = 0; i < 4; i += 1) {
        war_player *p = &gs.players[i];
        csFree(&p->hand);
        csFree(&p->pile);
        p->has_picked_card = false;

        p->pile = csNew(52);
        csFill(&p->pile);
        csShuffle(&p->pile);
        csPush(&p->hand, csPop(&p->pile));
        csPush(&p->hand, csPop(&p->pile));
        csPush(&p->hand, csPop(&p->pile));
    }
}

int main() {
    InitWindow(800, 600, "War2");
    SetTargetFPS(60);

    wa.arrow[ARROW_UP] = LoadTexture("assets/dd/a_up.png");
    wa.arrow[ARROW_DOWN] = LoadTexture("assets/dd/a_down.png");
    wa.arrow[ARROW_LEFT] = LoadTexture("assets/dd/a_left.png");
    wa.arrow[ARROW_RIGHT] = LoadTexture("assets/dd/a_right.png");

    for (size_t i = 0; i < 4; i++) {
        assert(IsTextureValid(wa.arrow[i]) && "War2 Validate Loaded Texture");
    }

    gs.player_count = 4;

    for (size_t i = 0; i < 4; i++) {
        Controller_LoadDebuggingKeymap(&gs.players[i].controller, i);
    }

    War2NewGame();

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(WHITE);

        switch (gs.state) {
        case PickCard:
            UpdatePickCards();
            DrawPickCards();
            if (AllPlayersPickedACard()) {
                gs.state = Compare;
            }
            break;

        case Compare:
            UpdateCompare();
            DrawCompare();
            if (compare_state.cards_to_show >= gs.player_count) {
                gs.state = PickCard;
                memset(&compare_state, 0, sizeof(compare_state));
                for (size_t i = 0; i < gs.player_count; i++) {
                    war_player *p = &gs.players[i];
                    p->has_picked_card = false;
                    p->selected = (Card){0};
                    if (csHasCard(&p->pile)) {
                        csPush(&p->hand, csPop(&p->pile));
                    }
                }
            }
            break;

        default:
            break;
        }

        EndDrawing();
    }
}

void csFree(CardStack *this) {
    if (this->cards)
        free(this->cards);
    this->cards = NULL;
    this->cap = this->count = 0;
}

Card csPop(CardStack *this) {
    assert(this->cards != NULL);
    assert(this->count > 0);

    this->count -= 1;
    return this->cards[this->count];
}

void csPush(CardStack *this, Card c) {
    if (this->count + 1 >= this->cap) {
        this->cap *= 2;
        this->cards = realloc(this->cards, (this->cap + 1) * sizeof(Card));
        assert(this->cards != NULL && "realloc(this->cards, ");
    }

    this->cards[this->count] = c; // bug: out of bounds ?
    this->count += 1;
}

bool csHasCard(CardStack *this) { return this->count > 0; }

void csShuffle(CardStack *this) {
    CardStack piles[5] = {0};
    for (int i = 0; i < sizeof(piles) / sizeof(piles[0]); i += 1) {
        piles[i] = csNew(10);
    }

    int shuffle_times = GetRandomValue(5, 30);

    for (int times = 0; times <= shuffle_times; times += 1) {
        while (csHasCard(this)) {
            Card c = csPop(this);
            int idx = GetRandomValue(0, (sizeof(piles) / sizeof(piles[0])) - 1);
            csPush(&piles[idx], c);
        }
        assert(this->count == 0);
        for (int i = 0; i < sizeof(piles) / sizeof(piles[0]); i += 1) {
            while (csHasCard(&piles[i])) {
                Card c = csPop(&piles[i]);
                csPush(this, c);
            }
        }
        for (int i = 0; i < sizeof(piles) / sizeof(piles[0]); i += 1) {
            assert(piles[i].count == 0);
        }
    }

    for (int i = 0; i < sizeof(piles) / sizeof(piles[0]); i += 1) {
        csFree(&piles[i]);
    }
}

void csFill(CardStack *ret) {
#define PC(VAL) csPush(ret, (Card){VAL})
    PC(V_A);
    PC(V_2);
    PC(V_3);
    PC(V_4);
    PC(V_5);
    PC(V_6);
    PC(V_7);
    PC(V_8);
    PC(V_9);
    PC(V_10);
    PC(V_J);
    PC(V_Q);
    PC(V_K);
    PC(V_A);
    PC(V_2);
    PC(V_3);
    PC(V_4);
    PC(V_5);
    PC(V_6);
    PC(V_7);
    PC(V_8);
    PC(V_9);
    PC(V_10);
    PC(V_J);
    PC(V_Q);
    PC(V_K);
    PC(V_A);
    PC(V_2);
    PC(V_3);
    PC(V_4);
    PC(V_5);
    PC(V_6);
    PC(V_7);
    PC(V_8);
    PC(V_9);
    PC(V_10);
    PC(V_J);
    PC(V_Q);
    PC(V_K);
    PC(V_A);
    PC(V_2);
    PC(V_3);
    PC(V_4);
    PC(V_5);
    PC(V_6);
    PC(V_7);
    PC(V_8);
    PC(V_9);
    PC(V_10);
    PC(V_J);
    PC(V_Q);
    PC(V_K);
#undef PC
}

CardStack csNew(int init_cap) {
    CardStack ret = {0};
    ret.cap = init_cap;
    ret.cards = malloc(sizeof(Card) * init_cap);
    memset(ret.cards, 0, sizeof(Card) * init_cap);

    return ret;
}
