#ifndef LEVENSHTEINDISTANCE_H
#define LEVENSHTEINDISTANCE_H

#include <QMatrix>
#include <QStringList>

struct MacthRes {
    int start = 0;
    int end = 0;
    QStringList textDiff;
    QStringList pinyinDiff;
};

struct StepPair {
    QString raw;
    QString res;
};

struct CalcuRes {
    int edit_distance;
    QStringList text_res, pinyin_res;
    QList<StepPair> corresponding_texts, corresponding_characters;
};

struct FaRes {
    QString match_text, match_pinyin, text_step, pinyin_step;
};

typedef QVector<QVector<int>> matrix;

class LevenshteinDistance {
public:
    static FaRes find_similar_substrings(const QStringList &target, const QStringList &pinyin_list, QStringList text_list = {},
                                  const bool &del_tip = false, const bool &ins_tip = false,
                                  const bool &sub_tip = false);

private:
    static MacthRes find_best_matches(const QStringList &text_list, const QStringList &source_list,
                                      const QStringList &sub_list);
    static QStringList fill_step_out(const QList<StepPair> &pairs, const bool &del_tip,
                                     const bool &ins_tip, const bool &sub_tip);
    static matrix init_dp_matrix(const int &m, const int &n, const int &del_cost, const int &ins_cost);
    static int calculate_edit_distance_dp(matrix dp, const QStringList &substring, const QStringList &target,
                                          const bool &del_cost, const bool &ins_cost, const bool &sub_cost);
    static QPair<QList<StepPair>, QList<StepPair>> backtrack_corresponding(matrix dp, const QStringList &text,
                                                                           const QStringList &substring,
                                                                           const QStringList &target);
    static CalcuRes calculate_edit_distance(const QStringList &_text, const QStringList &substring,
                                            const QStringList &target, const int &del_cost = 1, const int &ins_cost = 3,
                                            const int &sub_cost = 6);
    static CalcuRes findMinEditDistance(const QList<CalcuRes>& similar_substrings);
};

#endif // LEVENSHTEINDISTANCE_H
