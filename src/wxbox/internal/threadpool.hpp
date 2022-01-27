#ifndef __WXBOX_INTERNAL_THREAD_POOL_H
#define __WXBOX_INTERNAL_THREAD_POOL_H

#include <functional>
#include <QThreadPool>

namespace wxbox {
    namespace internal {
        class TaskInThreadPool final : public QRunnable
        {
          public:
            typedef std::function<void(void)> TaskFunc;

            explicit TaskInThreadPool(TaskFunc func)
              : func(func)
            {
            }

            void run() Q_DECL_OVERRIDE
            {
                if (func) {
                    func();
                }
            }

            static TaskInThreadPool* NewTask(TaskFunc func)
            {
                return new TaskInThreadPool(func);
            }

            static void StartTask(TaskFunc func)
            {
                QThreadPool::globalInstance()->start(new TaskInThreadPool(func));
            }

          private:
            TaskFunc func;
        };
    }
}

#endif  // #ifndef __WXBOX_INTERNAL_THREAD_POOL_H