#include "SmartHighlighter.h"

namespace LyricFA {

    SmartHighlighter::SmartHighlighter(const SequenceAligner &aligner) : m_aligner(aligner) {
    }

    std::tuple<QString, QString, QString, int>
        SmartHighlighter::highlight_differences(const QString &asr_result, const QString &match_phonetic,
                                                const QString &match_text) const {
        QVector<QString> asr_tokens = asr_result.split(' ', Qt::SkipEmptyParts).toVector();
        const QVector<QString> match_phonetic_tokens = match_phonetic.split(' ', Qt::SkipEmptyParts).toVector();
        QVector<QString> match_text_tokens = match_text.split(' ', Qt::SkipEmptyParts).toVector();

        if (match_phonetic_tokens.isEmpty()) {
            QStringList hl_asr;
            for (const QString &t : asr_tokens)
                hl_asr << "(" + t + ")";
            return {hl_asr.join(' '), QString(), QString(), asr_tokens.size()};
        }

        auto [edit_distance, aligned_asr, aligned_match] =
            m_aligner.compute_alignment(asr_tokens, match_phonetic_tokens);

        QStringList asr_highlighted, phonetic_highlighted, text_highlighted;
        int text_index = 0;
        const int text_tokens_len = match_text_tokens.size();

        for (int i = 0; i < aligned_asr.size(); ++i) {
            const QString &asr_token = aligned_asr[i];
            const QString &match_token = aligned_match[i];
            QString text_token;
            if (match_token != "-" && text_index < text_tokens_len) {
                text_token = match_text_tokens[text_index];
                ++text_index;
            }

            if (asr_token == "-") {
                phonetic_highlighted << "(" + match_token + ")";
                if (!text_token.isEmpty())
                    text_highlighted << "(" + text_token + ")";
            } else if (match_token == "-") {
                asr_highlighted << "(" + asr_token + ")";
            } else if (asr_token != match_token) {
                asr_highlighted << "(" + asr_token + ")";
                phonetic_highlighted << "(" + match_token + ")";
                if (!text_token.isEmpty())
                    text_highlighted << "(" + text_token + ")";
            } else {
                asr_highlighted << asr_token;
                phonetic_highlighted << match_token;
                if (!text_token.isEmpty())
                    text_highlighted << text_token;
            }
        }

        return {asr_highlighted.join(' '), phonetic_highlighted.join(' '), text_highlighted.join(' '), edit_distance};
    }

} // namespace LyricFA