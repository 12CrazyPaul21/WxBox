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
            typedef std::function<void(void)> FinishFunc;

            explicit TaskInThreadPool(TaskFunc func, FinishFunc finishFunc = nullptr)
              : func(func)
              , finishFunc(finishFunc)
            {
            }

            void run() Q_DECL_OVERRIDE
            {
                try {
                    if (func) {
                        func();
                    }
                }
                catch (const std::exception& /*e*/) {
                }

                if (finishFunc) {
                    finishFunc();
                }
            }

            static inline TaskInThreadPool* NewTask(TaskFunc func, FinishFunc finishFunc = nullptr)
            {
                return new TaskInThreadPool(func, finishFunc);
            }

            static inline void StartTask(TaskFunc func, FinishFunc finishFunc = nullptr)
            {
                QThreadPool::globalInstance()->start(new TaskInThreadPool(func, finishFunc));
            }

          private:
            TaskFunc   func;
            FinishFunc finishFunc;
        };
    }
}

#endif  // #ifndef __WXBOX_INTERNAL_THREAD_POOL_H