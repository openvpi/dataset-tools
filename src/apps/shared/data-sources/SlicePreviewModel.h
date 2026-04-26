#pragma once

#include <QString>
#include <vector>

namespace dstools {

    class IEditorDataSource;

    struct SlicePreviewRow {
        QString name;
        QString phSeq;
        QString phDur;
        QString phNum;
        QString noteSeq;
        QString noteDur;
        QString wordSpan;
        QString dirty;
        bool hasMissing = false;
    };

    class SlicePreviewModel {
    public:
        void setDataSource(IEditorDataSource *source);

        const std::vector<SlicePreviewRow> &rows() const {
            return m_rows;
        }
        bool isEmpty() const {
            return m_rows.empty();
        }

        void refresh();

        void invalidate();

    private:
        IEditorDataSource *m_source = nullptr;
        std::vector<SlicePreviewRow> m_rows;
        bool m_valid = false;

        void buildRows();
    };

} // namespace dstools