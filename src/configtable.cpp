#include "global.h"
#include "player.h"
#include "problem.h"
#include "configtable.h"

#include <QListView>
#include <QHeaderView>
#include <QStandardItemModel>
#include <QComboBox>
#include <QScrollBar>
#include <QDoubleSpinBox>
#include <QMouseEvent>
#include <QApplication>

using namespace std;

QRect ConfigTableItemDelegate::checkBoxRect(const QStyleOptionViewItem& viewItemStyleOptions)
{
    QStyleOptionButton checkBoxStyleOptionButton;
    QRect rect = QApplication::style()->subElementRect(QStyle::SE_CheckBoxIndicator, &checkBoxStyleOptionButton);
    QPoint point(viewItemStyleOptions.rect.x() + viewItemStyleOptions.rect.width()  / 2 - rect.width()  / 2,
                 viewItemStyleOptions.rect.y() + viewItemStyleOptions.rect.height() / 2 - rect.height() / 2);
    return QRect(point, rect.size());
}



QWidget* ConfigTableItemDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex& index) const
{
    switch (index.row())
    {
        case 0: // Type
        {
            QComboBox* editor = new QComboBox(parent);
            QListView* view = new QListView(parent);
            QStandardItemModel* model = new QStandardItemModel(parent);

            QStandardItem* item;
            item = new QStandardItem("传统型");
            item->setToolTip("传统型");
            model->appendRow(item);
            item = new QStandardItem("提交答案型");
            item->setToolTip("提交答案型");
            model->appendRow(item);

            editor->setView(view);
            editor->setModel(model);
            return editor;
        }
        case 3: // Checker
        {
            QComboBox* editor = new QComboBox(parent);
            QListView* view = new QListView(parent);
            QStandardItemModel* model = new QStandardItemModel(parent);
            auto& internal_checker = Problem::INTERNAL_CHECKER_MAP;

            QStandardItem* item;
            for (auto i : internal_checker)
            {
                auto checker = i.second;
                item = new QStandardItem(checker.first);
                item->setToolTip(checker.second);
                item->setFont(Global::BOLD_FONT);
                model->appendRow(item);
            }

            QStringList dirs = { QDir().currentPath() + "/checker",
                                 Global::g_contest.data_path + problem_list[index.column()]
                               };
            for (auto dir : dirs)
            {
#ifdef Q_OS_WIN
                QStringList list = QDir(dir).entryList(QDir::Files);
#else
                QStringList list = QDir(dir).entryList(QDir::Files | QDir::Executable);
#endif
                for (auto checker : list)
                {
#ifdef Q_OS_WIN
                    if (!checker.endsWith(".exe")) continue;
#endif
                    if (internal_checker.find(Problem::RemoveFileExtension(checker)) != internal_checker.end()) continue;

                    item = new QStandardItem(checker);
                    item->setToolTip(QString("%1 (位置: %2)").arg(checker, dir));
                    model->appendRow(item);
                }
            }

            editor->setView(view);
            editor->setModel(model);
            return editor;
        }
        case 1: // Time Limit
        {
            QDoubleSpinBox* editor = new QDoubleSpinBox(parent);
            editor->setAlignment(Qt::AlignCenter);
            editor->setDecimals(1);
            editor->setMinimum(0);
            editor->setMaximum(3600);
            return editor;
        }
        case 2: // Memory Limit
        {
            QDoubleSpinBox* editor = new QDoubleSpinBox(parent);
            editor->setAlignment(Qt::AlignCenter);
            editor->setDecimals(1);
            editor->setMinimum(0);
            editor->setMaximum(8192);
            editor->setSingleStep(128);
            return editor;
        }
        default:
            return nullptr;
    }
}

void ConfigTableItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (index.row() != 4)
    {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    QStyleOptionButton checkBoxOption;
    int checked = index.model()->data(index, Qt::DisplayRole).toInt();
    if (checked != 2)
        checkBoxOption.state |= QStyle::State_Enabled;
    if (checked)
        checkBoxOption.state |= QStyle::State_On;
    else
        checkBoxOption.state |= QStyle::State_Off;
    checkBoxOption.rect = checkBoxRect(option);

    QApplication::style()->drawControl(QStyle::CE_CheckBox, &checkBoxOption, painter);
}

void ConfigTableItemDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    switch (index.row())
    {
        case 0: // Type
        case 3: // Checker
        {
            QComboBox* edit = static_cast<QComboBox*>(editor);
            int p = edit->findText(index.model()->data(index, Qt::DisplayRole).toString());
            if (p == -1) p = 0;
            edit->setCurrentIndex(p);
            return;
        }
        case 1: // Time Limit
        case 2: // Memory Limit
        {
            QDoubleSpinBox* edit = static_cast<QDoubleSpinBox*>(editor);
            bool ok;
            edit->setValue(index.model()->data(index, Qt::DisplayRole).toDouble(&ok));
            if (!ok) edit->setValue(index.row() == 1 ? 1 : 128);
            return;
        }
    }
}

void ConfigTableItemDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    switch (index.row())
    {
        case 0: // Type
        case 3: // Checker
        {
            QComboBox* edit = static_cast<QComboBox*>(editor);
            model->setData(index, edit->currentText());
            return;
        }
        case 1: // Time Limit
        case 2: // Memory Limit
        {
            QDoubleSpinBox* edit = static_cast<QDoubleSpinBox*>(editor);
            model->setData(index, edit->value());
            return;
        }
    }
}

bool ConfigTableItemDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    if (index.row() != 4)
        return QStyledItemDelegate::editorEvent(event, model, option, index);

    if (event->type() == QEvent::MouseButtonRelease)
    {
        QMouseEvent* mouse_event = static_cast<QMouseEvent*>(event);
        if (mouse_event->button() == Qt::LeftButton && checkBoxRect(option).contains(mouse_event->pos()))
        {
            int checked = index.model()->data(index, Qt::DisplayRole).toInt();
            model->setData(index, checked <= 1 ? !checked : 2, Qt::DisplayRole);
        }
    }
    return false;
}





ConfigTable::ConfigTable(const QStringList& list, QWidget* parent) : QTableView(parent),
    model(new QStandardItemModel(this)), problem_list(list), is_changing_data(false)
{
    this->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    this->setFocusPolicy(Qt::NoFocus);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setAlternatingRowColors(true);
    this->setSelectionMode(QAbstractItemView::NoSelection);
    this->setStyleSheet(QLatin1String("QTableView\n"
                                      "{\n"
                                      "background-color:#F8F8F8;\n"
                                      "alternate-background-color:#FFFFFF;\n"
                                      "}\n"
                                      ""));

    this->horizontalHeader()->setSectionsMovable(true);
    this->horizontalHeader()->setTextElideMode(Qt::ElideRight);
    this->horizontalHeader()->setHighlightSections(false);
    this->horizontalHeader()->setDefaultSectionSize(85);
    this->horizontalHeader()->setMinimumSectionSize(85);
    this->horizontalHeader()->setFixedHeight(25);
    this->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    this->verticalHeader()->setHighlightSections(false);
    this->verticalHeader()->setDefaultSectionSize(27);
    this->verticalHeader()->setMinimumSectionSize(27);
    this->verticalHeader()->setFixedWidth(80);
    this->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    this->setModel(model);
    this->setItemDelegate(new ConfigTableItemDelegate(problem_list, this));

    loadProblems();

    connect(model, &QAbstractItemModel::dataChanged, this, &ConfigTable::onDataChanged);
}

ConfigTable::~ConfigTable()
{

}

void ConfigTable::setModelDataNew(int c)
{
    SetItemText(0, c, "传统型");
    SetItemData(1, c, 1);
    SetItemData(2, c, 128);
    SetItemText(3, c, "全文比较");
    SetItemData(4, c, 2); // 0:off; 1:on; 2:on|disable

    for (int i = 0; i < 4; i++)
    {
        SetItemChanged(i, c);
        model->item(i, c)->setEditable(true);
    }
}

void ConfigTable::setModelData(int c)
{
    Problem* problem = nullptr;
    if (c < Global::g_contest.problem_num) problem = Global::g_contest.problems[c];

    if (!problem || !problem->TestCaseCount())
    {
        setModelDataNew(c);
        return;
    }

    SetItemData(4, c, 0);
    SetItemText(3, c, problem->InternalCheckerName());
    switch (problem->Type())
    {
        case Global::Traditional:
        {
            SetItemText(0, c, "传统型");
            double minT = 1e9, maxT = 0, minM = 1e9, maxM = 0;
            for (int i = 0; i < problem->TestCaseCount(); i++)
            {
                TestCase* point = problem->TestCaseAt(i);
                minT = min(minT, point->TimeLimit()), maxT = max(maxT, point->TimeLimit());
                minM = min(minM, point->MemoryLimit()), maxM = max(maxM, point->MemoryLimit());
            }
            if (minT > maxT || minM > maxM)
            {
                setModelDataNew(c);
                return;
            }

            if (minT == maxT)
                SetItemData(1, c, minT);
            else if (0 <= minT && maxT <= 3600)
                SetItemText(1, c, QString("%1~%2").arg(minT).arg(maxT));
            else
            {
                SetItemText(1, c, QString("无效"));
                SetItemBold(1, c);
            }

            if (minM == maxM)
                SetItemData(2, c, minM);
            else if (0 <= minM && maxM <= 8192)
                SetItemData(2, c, QString("%1~%2").arg(minM).arg(maxM));
            else
            {
                SetItemText(2, c, QString("无效"));
                SetItemBold(2, c);
            }
            break;
        }
        case Global::AnswersOnly:
        {
            SetItemText(0, c, QString("提交答案型"));
            SetItemText(1, c, "");
            SetItemText(2, c, "");
            model->item(1, c)->setEditable(false);
            model->item(2, c)->setEditable(false);
            break;
        }
        default:
        {
            SetItemText(0, c, QString("无效"));
            SetItemBold(0, c);
            SetItemText(1, c, "");
            SetItemText(2, c, "");
            model->item(1, c)->setEditable(false);
            model->item(2, c)->setEditable(false);
            break;
        }
    }
}

void ConfigTable::loadProblems()
{
    model->setRowCount(5);
    model->setVerticalHeaderLabels({"题目类型", "时间限制", "内存限制", "比较方式", "清空原配置"});
    model->verticalHeaderItem(0)->setToolTip("试题的类型。");
    model->verticalHeaderItem(1)->setToolTip("试题每个测试点拥有的运行时间上限(仅限传统型试题)。单位: 秒(s)");
    model->verticalHeaderItem(2)->setToolTip("试题每个测试点拥有的运行内存上限(仅限传统型试题)。单位: 兆字节(MB)");
    model->verticalHeaderItem(3)->setToolTip("选手程序输出文件(或答案文件)与标准输出文件的比较方式。");
    model->verticalHeaderItem(4)->setToolTip("清空原来的所有配置。");
    for (int i = 0; i < 5; i++) model->verticalHeaderItem(i)->setTextAlignment(Qt::AlignCenter);

    int num = problem_list.size();
    model->setColumnCount(num);
    model->setHorizontalHeaderLabels(problem_list);
    for (int i = 0; i < num; i++)
    {
        model->horizontalHeaderItem(i)->setToolTip(problem_list[i]);
        setModelData(i);
    }

    int w = min(max(num, 3), 12) * this->horizontalHeader()->defaultSectionSize() + this->verticalHeader()->width() + 2 * this->frameWidth();
    int h = 5 * this->verticalHeader()->defaultSectionSize() + this->horizontalHeader()->height() + 2 * this->frameWidth();
    if (num > 12) h += this->horizontalScrollBar()->sizeHint().height();
    this->setFixedSize(w, h);
}



void ConfigTable::onDataChanged(const QModelIndex& topLeft, const QModelIndex&)
{
    if (is_changing_data) return;
    is_changing_data = true;

    int r = topLeft.row(), c = topLeft.column();
    SetItemText(r, c, ItemText(r, c));
    SetItemChanged(r, c);

    switch (r)
    {
        case 0: // Type
        {
            if (ItemText(r, c) == "提交答案型")
            {
                SetItemText(1, c, "");
                SetItemText(2, c, "");
                model->item(1, c)->setEditable(false);
                model->item(2, c)->setEditable(false);
            }
            else if (ItemText(r, c) == "传统型")
            {
                SetItemData(1, c, 1);
                SetItemData(2, c, 128);
                SetItemChanged(1, c);
                SetItemChanged(2, c);
                model->item(1, c)->setEditable(true);
                model->item(2, c)->setEditable(true);
            }
            break;
        }
        case 4: // Clean
        {
            if (c >= Global::g_contest.problem_num)
                setModelDataNew(c);
            else if (ItemData(r, c).toBool())
            {
                setModelDataNew(c);
                SetItemData(4, c, 1);
            }
            break;
        }
        default:
            break;
    }
    is_changing_data = false;
}
