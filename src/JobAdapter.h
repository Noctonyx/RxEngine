//
// Created by shane on 4/06/2021.
//

#ifndef INDUSTRONAUT_JOBADAPTER_H
#define INDUSTRONAUT_JOBADAPTER_H



namespace RxEngine {

    struct RxJobAdaptor : ecs::JobInterface
            {
        JobHandle create(std::function<void()> f) override
        {
            return RxCore::CreateJob<void>(f);
        }

        void schedule(JobHandle job_handle) override
        {
            auto x = std::static_pointer_cast<RxCore::Job<void>>(job_handle);
            x->schedule();
        }

        bool isComplete(JobHandle job_handle) const override
        {
            auto x = std::static_pointer_cast<RxCore::Job<void>>(job_handle);
            return x->isCompleted();
        }

        void awaitCompletion(JobHandle job_handle) override
        {
            auto x = std::static_pointer_cast<RxCore::Job<void>>(job_handle);
            x->waitComplete();
        }
            };

}
#endif //INDUSTRONAUT_JOBADAPTER_H
