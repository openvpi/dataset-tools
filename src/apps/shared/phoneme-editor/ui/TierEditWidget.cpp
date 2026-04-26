#include "TierEditWidget.h"

#include "BoundaryDragController.h"
#include "IntervalTierView.h"
#include "TextGridDocument.h"
#include "ChartCoordinate.h"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QRadioButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QVBoxLayout>
#include <dsfw/widgets/ViewportController.h>

namespace dstools {
    namespace phonemelabeler {

        TierEditWidget::TierEditWidget(TextGridDocument *doc, QUndoStack *undoStack, ViewportController *viewport,
                                       BoundaryDragController *dragController, QWidget *parent) :
            QWidget(parent), m_document(doc), m_undoStack(undoStack), m_viewport(viewport),
            m_dragController(dragController) {
            // 创建主水平布局：左侧单选按钮 + 右侧层级视图
            auto *mainLayout = new QHBoxLayout(this);
            mainLayout->setContentsMargins(0, 0, 0, 0);
            mainLayout->setSpacing(0);

            // 创建单选按钮容器
            m_radioButtonContainer = new QWidget(this);
            m_radioButtonContainer->setFixedWidth(0); // 初始为0，将在setYAxisWidth中设置
            auto *radioLayout = new QVBoxLayout(m_radioButtonContainer);
            radioLayout->setContentsMargins(0, 0, 0, 0);
            radioLayout->setSpacing(0);
            radioLayout->addStretch(); // 顶部留空，与层级视图对齐

            // 创建按钮组
            m_buttonGroup = new QButtonGroup(this);
            m_buttonGroup->setExclusive(true);

            // 创建层级视图容器
            m_tierViewsContainer = new QWidget(this);
            m_layout = new QVBoxLayout(m_tierViewsContainer);
            m_layout->setContentsMargins(0, 0, 0, 0);
            m_layout->setSpacing(0);

            // 添加到主布局
            mainLayout->addWidget(m_radioButtonContainer);
            mainLayout->addWidget(m_tierViewsContainer, 1); // 右侧容器可拉伸

            if (m_viewport) {
                connect(m_viewport, &ViewportController::viewportChanged, this, &TierEditWidget::onViewportChanged);
                m_viewStart = m_viewport->state().startSec;
                m_viewEnd = m_viewport->state().endSec;
            }

            if (m_document) {
                rebuildTierViews();
            }
        }

        TierEditWidget::~TierEditWidget() = default;

        void TierEditWidget::setDocument(TextGridDocument *doc) {
            m_document = doc;
            rebuildTierViews();
        }

        void TierEditWidget::setViewport(const ViewportState &state) {
            m_viewStart = state.startSec;
            m_viewEnd = state.endSec;

            for (auto *view : m_tierViews) {
                view->setViewport(state);
            }
        }

        void TierEditWidget::onViewportChanged(const ViewportState &state) {
            setViewport(state);
        }

        void TierEditWidget::rebuildTierViews() {
            // Clear existing views and radio buttons
            qDeleteAll(m_tierViews);
            m_tierViews.clear();

            qDeleteAll(m_radioButtons);
            m_radioButtons.clear();

            if (m_buttonGroup) {
                delete m_buttonGroup;
                m_buttonGroup = new QButtonGroup(this);
                m_buttonGroup->setExclusive(true);
            }

            if (!m_document)
                return;

            int count = m_document->tierCount();
            for (int i = 0; i < count; ++i) {
                if (m_document->isIntervalTier(i)) {
                    // 创建单选按钮
                    auto *radioButton = new QRadioButton(m_radioButtonContainer);
                    radioButton->setFixedSize(24, 24);
                    radioButton->setChecked(i == m_document->activeTierIndex());
                    m_buttonGroup->addButton(radioButton, i);
                    m_radioButtons.append(radioButton);

                    // 连接单选按钮信号
                    connect(radioButton, &QRadioButton::clicked, this, [this, i]() {
                        if (m_document) {
                            m_document->setActiveTierIndex(i);
                            // 更新所有层级视图的激活状态
                            for (auto *view : m_tierViews) {
                                view->setActive(view->tierIndex() == i);
                            }
                            emit tierActivated(i);
                        }
                    });

                    // 创建层级视图
                    auto *view = new IntervalTierView(i, m_document, m_undoStack, m_viewport, m_dragController,
                                                      m_tierViewsContainer);
                    view->setActive(i == m_document->activeTierIndex());
                    if (m_converter) view->setCoordConverter(m_converter);
                    // Y轴宽度将在setYAxisWidth中统一设置

                    connect(view, &IntervalTierView::activated, this, [this](int tierIndex) {
                        if (m_document) {
                            m_document->setActiveTierIndex(tierIndex);
                            // 更新单选按钮状态
                            if (tierIndex >= 0 && tierIndex < m_radioButtons.size()) {
                                m_radioButtons[tierIndex]->setChecked(true);
                            }
                            // 更新所有层级视图的激活状态
                            for (auto *v : m_tierViews) {
                                v->setActive(v->tierIndex() == tierIndex);
                            }
                            emit tierActivated(tierIndex);
                        }
                    });

                    connect(view, &IntervalTierView::hoveredBoundaryChanged, this,
                            [this, i](int boundaryIndex) { emit tierHoveredBoundaryChanged(i, boundaryIndex); });

                    connect(view, &IntervalTierView::requestPlayback, this,
                            [this](TimePos startTime, TimePos endTime) { emit requestPlayback(startTime, endTime); });

                    m_layout->addWidget(view);
                    m_tierViews.append(view);
                }
            }

            // 更新单选按钮布局
            updateRadioButtons();
        }

        void TierEditWidget::updateRadioButtons() {
            if (!m_radioButtonContainer)
                return;

            // 获取单选按钮容器的布局
            auto *radioLayout = qobject_cast<QVBoxLayout *>(m_radioButtonContainer->layout());
            if (!radioLayout)
                return;

            // 清除现有按钮（保留顶部留空）
            while (radioLayout->count() > 1) {
                QLayoutItem *item = radioLayout->takeAt(1);
                if (item && item->widget()) {
                    item->widget()->setParent(nullptr);
                }
                delete item;
            }

            // 添加单选按钮
            for (auto *radioButton : m_radioButtons) {
                radioLayout->addWidget(radioButton);
            }

            // 添加底部留空
            radioLayout->addStretch();
        }

        void TierEditWidget::resizeEvent(QResizeEvent *event) {
            QWidget::resizeEvent(event);
        }

        void TierEditWidget::setCoordConverter(const ChartCoordinate *conv) {
            m_converter = conv;
            for (auto *view : m_tierViews) {
                view->setCoordConverter(conv);
            }
        }

    } // namespace phonemelabeler
} // namespace dstools
