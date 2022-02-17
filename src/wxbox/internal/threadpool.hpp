#ifndef __WXBOX_INTERNAL_THREAD_POOL_H
#define __WXBOX_INTERNAL_THREAD_POOL_H

#include <QThreadPool>

#include <functional>
#include <future>

namespace wxbox {
    namespace internal {

        class Future final
        {
          public:
            Future(std::future<void> f)
              : objFuture(std::move(f))
            {
            }

            void wait()
            {
                while (objFuture.wait_for(std::chrono::milliseconds(10)) == std::future_status::timeout) {
                    QCoreApplication::processEvents();
                }
            }

          private:
            std::future<void> objFuture;
        };

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

                sigFinished.set_value();
            }

            Future get_future()
            {
                return Future(sigFinished.get_future());
            }

            static inline TaskInThreadPool* NewTask(TaskFunc func, FinishFunc finishFunc = nullptr)
            {
                return new TaskInThreadPool(func, finishFunc);
            }

            static inline Future StartTask(TaskFunc func, FinishFunc finishFunc = nullptr)
            {
                TaskInThreadPool* th = new TaskInThreadPool(func, finishFunc);
                QThreadPool::globalInstance()->start(th);
                return th->get_future();
            }

          private:
            TaskFunc           func;
            FinishFunc         finishFunc;
            std::promise<void> sigFinished;
        };
    }
}

#endif  // #ifndef __WXBOX_INTERNAL_THREAD_POOL_H