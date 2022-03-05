#ifndef __WXBOX_INTERNAL_TASK_H
#define __WXBOX_INTERNAL_TASK_H

#include <QCoreApplication>
#include <QThread>
#include <QFuture>

#include <functional>
#include <thread>
#include <chrono>

namespace wxbox {
    namespace internal {

        class AsyncTask final
        {
          public:
            typedef std::function<void(void)> TaskFunc;

          private:
            explicit AsyncTask(TaskFunc func)
              : func(func)
            {
                if (!func) {
                    return;
                }

                future = QtConcurrent::run(func);
            }

          public:
            void wait()
            {
                if (!func) {
                    return;
                }

                while (!future.isFinished()) {
                    QCoreApplication::processEvents();
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }

            static inline AsyncTask start(TaskFunc func)
            {
                return AsyncTask(func);
            }

          private:
            QFuture<void> future;
            TaskFunc      func;
        };
    }
}

#endif  // #ifndef __WXBOX_INTERNAL_TASK_H