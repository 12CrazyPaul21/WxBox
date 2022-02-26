#ifndef __X_STYLE_COMMAND_LINE_H
#define __X_STYLE_COMMAND_LINE_H

#include <QLineEdit>
#include <QKeyEvent>

#include <deque>

#include <xstyle/xstyle.h>
#include <xstyle/xstylecommon.h>

//
// XStyleCommandLine
//

class XStyleCommandLine : public QLineEdit
{
    Q_OBJECT

    using FnExecuteCommandHandler = std::function<void(const QString& command)>;

  public:
    explicit XStyleCommandLine(QWidget* parent = nullptr)
      : QLineEdit(parent)
      , maxLine(100)
      , cursor(0)
      , handler(nullptr)
    {
        QObject::connect(this, &QLineEdit::returnPressed, this, &XStyleCommandLine::Execute);
    }

    int MaxHistoryLine() const
    {
        return maxLine;
    }

    void SetMaxHistoryLine(size_t maxline)
    {
        maxLine = qMax(size_t(1), maxline);
        Balance();
    }

    FnExecuteCommandHandler Handler() const
    {
        return handler;
    }

    void RegisterExecuteHandler(FnExecuteCommandHandler fn)
    {
        handler = fn;
    }

    void LoadHistory(const std::vector<std::string>& collection)
    {
        history.clear();

        for (auto item : collection) {
            history.push_back(item.c_str());

            if (history.size() >= maxLine) {
                break;
            }
        }

        cursor = history.size();
        Balance();
    }

    std::vector<std::string> SaveHistory(const size_t maxPersistenceLine)
    {
        size_t index = 0;
        if (history.size() > maxPersistenceLine) {
            index = history.size() - maxPersistenceLine;
        }

        std::vector<std::string> result;
        for (; index < history.size(); index++) {
            result.push_back(history.at(index).toStdString());
        }

        return result;
    }

    void Balance()
    {
        if (history.size() > maxLine) {
            auto deleteCount = history.size() - maxLine;
            history.erase(history.begin(), history.begin() + deleteCount);
            cursor = maxLine;
        }
    }

    void PrevCommand()
    {
        if (cursor <= 0) {
            return;
        }

        if (cursor == history.size()) {
            tmp = text();
        }

        setText(history.at(--cursor));
    }

    void NextCommand()
    {
        if (cursor >= history.size()) {
            return;
        }

        if (++cursor < history.size()) {
            setText(history.at(cursor));
        }
        else {
            setText(tmp);
        }
    }

  protected:
    void Execute()
    {
        if (!handler) {
            return;
        }

        auto statement = text();
        if (statement.isEmpty()) {
            return;
        }

        handler(statement);

        if (cursor != history.size() - 1) {
            history.push_back(statement);
            cursor = history.size();
            Balance();
        }
    }

    void keyPressEvent(QKeyEvent* event)
    {
        QLineEdit::keyPressEvent(event);

        switch (event->key()) {
            case Qt::Key_Up: {
                PrevCommand();
                break;
            }

            case Qt::Key_Down: {
                NextCommand();
                break;
            }

            case Qt::Key_Left:
            case Qt::Key_Right:
                break;

            case Qt::Key_Escape:
                clear();
            default:
                cursor = history.size();
                break;
        }
    }

  protected:
    size_t                  maxLine;
    size_t                  cursor;
    QString                 tmp;
    std::deque<QString>     history;
    FnExecuteCommandHandler handler;
};

#endif  // #ifndef __X_STYLE_COMMAND_LINE_H