#include "Zakazane/ExecutionGraph/JobIdTraits.h"
#include "Zakazane/ExecutionGraph/Scheduler.h"
#include "Zakazane/ExecutionGraph/SynchronizationTraits.h"
#include "Zakazane/Test/Test.h"

namespace Zkz::ExecutionGraph::Test
{

namespace SchedulerTestPrivate
{

DECLARE_LOG_CATEGORY_EXTERN(LogExecutionGraphTest, Log, All);
DEFINE_LOG_CATEGORY(LogExecutionGraphTest);

template <class SchedulerType>
bool TestEnqueueStage(
	FAutomationTestBase& Test,
	const FString& Prefix,
	SchedulerType& Scheduler,
	typename SchedulerType::JobIdType JobId,
	TConstArrayView<typename SchedulerType::JobIdReferenceType> Predecessors)
{
	const auto Result = Scheduler.EnqueueStage(MoveTemp(JobId), MoveTemp(Predecessors));
	return Test.TestTrue(Prefix + "Enqueue stage succeeds", Result.HasValue());
}

template <class SchedulerType>
TOptional<FFutureTaskExecution> TestEnqueueTask(
	FAutomationTestBase& Test,
	const FString& Prefix,
	SchedulerType& Scheduler,
	typename SchedulerType::JobIdType JobId,
	TConstArrayView<typename SchedulerType::JobIdReferenceType> Predecessors)
{
	auto Result = Scheduler.EnqueueTask(MoveTemp(JobId), MoveTemp(Predecessors));
	if (Test.TestTrue(Prefix + "Enqueue task succeeds", Result.HasValue()))
	{
		return MoveTemp(Result).GetValue();
	}

	return NullOpt;
}

template <class SchedulerType>
bool TestJobState(
	FAutomationTestBase& Test,
	const FString& Prefix,
	const SchedulerType& Scheduler,
	typename SchedulerType::JobIdType JobId,
	const EZkzExecutionGraphJobStateId ExpectedJobStateId)
{
	ZKZ_RETURN_IF(!Test.TestTrue(Prefix + "Job exists", Scheduler.HasJob(JobId)), false);

	const EZkzExecutionGraphJobStateId TaskJobStateId = Scheduler.WithJob(JobId, &FJob::GetJobStateId);
	return Test.TestEqual(Prefix + "Job in expected state", TaskJobStateId, ExpectedJobStateId);
}

void TestTaskExecution(FAutomationTestBase& Test, const FString& Prefix, FFutureTaskExecution FutureTaskExecution)
{
	FutureTaskExecution.Next(
		[&Test, Prefix](TCancelableFutureResult<FTaskArgs> TaskArgsResult)
		{
			if (!Test.TestTrue(Prefix + "Executed successfully", !TaskArgsResult.HasError()))
			{
				return;
			}

			TaskArgsResult->CompletionPromise.EmplaceValue();
		});
}

void TestDelayedTaskExecution(
	FAutomationTestBase& Test,
	const FString& Prefix,
	FFutureTaskExecution FutureTaskExecution,
	TOptional<FJobCompletionPromise>& OutJobCompletionPromise)
{
	FutureTaskExecution.Next(
		[&OutJobCompletionPromise, &Test, Prefix](TCancelableFutureResult<FTaskArgs> TaskArgsResult)
		{
			if (!Test.TestTrue(Prefix + "Executed successfully", !TaskArgsResult.HasError()))
			{
				return;
			}

			OutJobCompletionPromise = MoveTemp(TaskArgsResult).GetValue().CompletionPromise;
		});
}

template <class SchedulerType>
bool TestCloseStage(
	FAutomationTestBase& Test, const FString& Prefix, SchedulerType& Scheduler, typename SchedulerType::JobIdType JobId)
{
	auto Result = Scheduler.CloseStage(JobId);
	return Test.TestTrue(Prefix + "Close stage succeeds", !Result.HasError());
}

template <class SynchronizationPrimitiveType>
void GeneratesUniqueIds(FAutomationTestBase& Test, const FStringView& Prefix)
{
	using SchedulerType =
		TScheduler<decltype(LogExecutionGraphTest), TDefaultSchedulerJobIdTraits<>, SynchronizationPrimitiveType>;

	SchedulerType Scheduler{LogExecutionGraphTest};

	const auto SourceJobId = Scheduler.MakeJobIdFromString(TEXTVIEW("Job.Name"));

	Test.TestEqual(
		Prefix + FString{"Pre MakeUnique returns job id without number"},
		TDefaultSchedulerJobIdTraits<>::ToString(SourceJobId),
		TEXT("Job.Name"));

	const auto JobId0 = Scheduler.MakeUnique(SourceJobId);

	auto Lock = Scheduler.Lock();
	const auto JobId1 = Scheduler.MakeUnique(SourceJobId, Lock);
	const auto JobId2 = Scheduler.MakeUnique(JobId0, Lock);

	Test.TestTrue(Prefix + FString{" JobId0 != SourceJob"}, JobId0 != SourceJobId);
	Test.TestTrue(Prefix + FString{" JobId2 != SourceJob"}, JobId1 != SourceJobId);
	Test.TestTrue(Prefix + FString{" JobId2 != JobId1"}, JobId1 != JobId0);
	Test.TestTrue(Prefix + FString{" JobId3 != SourceJob"}, JobId2 != SourceJobId);
	Test.TestTrue(Prefix + FString{" JobId3 != JobId1"}, JobId2 != JobId0);
	Test.TestTrue(Prefix + FString{" JobId3 != JobId2"}, JobId2 != JobId1);

	Test.TestEqual(
		Prefix + FString{"ToString returns job id 0"},
		TDefaultSchedulerJobIdTraits<>::ToString(JobId0),
		TEXT("Job.Name_0"));
	Test.TestEqual(
		Prefix + FString{"ToString returns job id 1"},
		TDefaultSchedulerJobIdTraits<>::ToString(JobId1),
		TEXT("Job.Name_1"));
}

template <class SynchronizationPrimitiveType>
void ParentStageThenChildTaskExecuteImmediately(FAutomationTestBase& Test, const FString& Prefix)
{
	using SchedulerType =
		TScheduler<decltype(LogExecutionGraphTest), TDefaultSchedulerJobIdTraits<>, SynchronizationPrimitiveType>;

	SchedulerType Scheduler{LogExecutionGraphTest};

	const auto ParentStageId = Scheduler.MakeJobIdFromString(TEXTVIEW("ParentStage1"));
	const auto ChildTaskId = Scheduler.MakeJobIdFromString(TEXTVIEW("ParentStage1.Task"));

	// Parent stage
	{
		TestEnqueueStage(Test, Prefix + TEXT("ParentStage: "), Scheduler, ParentStageId, {});

		ZKZ_RETURN_IF(!Test.TestTrue(Prefix + "ParentStage: Enqueued stage exists", Scheduler.HasJob(ParentStageId)));
		const EZkzExecutionGraphJobStateId JobStateId = Scheduler.WithJob(ParentStageId, &FJob::GetJobStateId);
		Test.TestEqual(
			Prefix + "ParentStage: Enqueued parent stage with no predecessors is immediately executing",
			JobStateId,
			EZkzExecutionGraphJobStateId::ExecutingStage);
	}

	// Child task - parent stage defined
	{
		auto OptFutureTaskExecution = TestEnqueueTask(Test, Prefix + TEXT("ChildTask: "), Scheduler, ChildTaskId, {});

		if (OptFutureTaskExecution.IsSet())
		{
			ZKZ_RETURN_IF(!Test.TestTrue(Prefix + "ChildTask: Enqueued task exists", Scheduler.HasJob(ChildTaskId)));
			const EZkzExecutionGraphJobStateId JobStateId = Scheduler.WithJob(ChildTaskId, &FJob::GetJobStateId);
			Test.TestEqual(
				Prefix
					+ "ChildTask: Enqueued child task with no predecessors and defined stage is immediately executing",
				JobStateId,
				EZkzExecutionGraphJobStateId::ExecutingTask);

			TestTaskExecution(Test, Prefix + TEXT("ChildTask: "), MoveTemp(*OptFutureTaskExecution));
		}
	}
}

template <class SynchronizationPrimitiveType>
void GrandchildTaskThenParentStagesWaitForDefinition(FAutomationTestBase& Test, const FString& Prefix)
{
	using SchedulerType =
		TScheduler<decltype(LogExecutionGraphTest), TDefaultSchedulerJobIdTraits<>, SynchronizationPrimitiveType>;

	SchedulerType Scheduler{LogExecutionGraphTest};

	const auto StageId = Scheduler.MakeJobIdFromString(TEXTVIEW("Stage"));
	const auto ChildStageId = Scheduler.MakeJobIdFromString(TEXTVIEW("Stage.ChildStage"));
	const auto GrandchildTaskId = Scheduler.MakeJobIdFromString(TEXTVIEW("Stage.ChildStage.GrandchildTask"));

	// [1] Grandchild task - child stage not defined
	{
		auto OptFutureTaskExecution =
			TestEnqueueTask(Test, Prefix + TEXT("GrandchildTask: "), Scheduler, GrandchildTaskId, {});
		ZKZ_RETURN_IF(!OptFutureTaskExecution.IsSet());

		TestJobState(
			Test,
			Prefix + TEXT("[1] GrandchildTask: "),
			Scheduler,
			GrandchildTaskId,
			EZkzExecutionGraphJobStateId::DefinedTask);
		TestJobState(
			Test, Prefix + TEXT("[1] ChildStage: "), Scheduler, ChildStageId, EZkzExecutionGraphJobStateId::StageStub);
		Test.TestFalse(Prefix + "[1] Stage: does not exist", Scheduler.HasJob(StageId));

		TestTaskExecution(Test, Prefix + TEXT("[1] GrandchildTask: "), MoveTemp(*OptFutureTaskExecution));
	}

	// [2] ChildStage - parent stage not defined
	{
		const bool bEnqueueSuccessful =
			TestEnqueueStage(Test, Prefix + TEXT("[2] ChildStage: "), Scheduler, ChildStageId, {});
		ZKZ_RETURN_IF(!bEnqueueSuccessful);

		TestJobState(
			Test,
			Prefix + TEXT("[2] GrandchildTask: "),
			Scheduler,
			GrandchildTaskId,
			EZkzExecutionGraphJobStateId::DefinedTask);
		TestJobState(
			Test,
			Prefix + TEXT("[2] ChildStage: "),
			Scheduler,
			ChildStageId,
			EZkzExecutionGraphJobStateId::DefinedStage);
		TestJobState(Test, Prefix + TEXT("[2] Stage: "), Scheduler, StageId, EZkzExecutionGraphJobStateId::StageStub);
	}

	// [3] Close stages - parent stage still not defined
	{
		TestCloseStage(Test, Prefix + TEXT("[3] ChildStage: "), Scheduler, ChildStageId);
		TestCloseStage(Test, Prefix + TEXT("[3] Stage: "), Scheduler, StageId);
	}

	// [4] Parent stage - all defined and closed at this point, should immediately trigger execution
	{
		const bool bEnqueueSuccessful = TestEnqueueStage(Test, Prefix + TEXT("[4] Stage: "), Scheduler, StageId, {});
		ZKZ_RETURN_IF(!bEnqueueSuccessful);

		TestJobState(
			Test,
			Prefix + TEXT("[4] GrandchildTask: "),
			Scheduler,
			GrandchildTaskId,
			EZkzExecutionGraphJobStateId::Completed);
		TestJobState(
			Test, Prefix + TEXT("[4] ChildStage: "), Scheduler, ChildStageId, EZkzExecutionGraphJobStateId::Completed);
		TestJobState(Test, Prefix + TEXT("[4] Stage: "), Scheduler, StageId, EZkzExecutionGraphJobStateId::Completed);
	}
}

template <class SynchronizationPrimitiveType>
void PredecessorsThenSuccessors(FAutomationTestBase& Test, const FString& Prefix)
{
	using SchedulerType =
		TScheduler<decltype(LogExecutionGraphTest), TDefaultSchedulerJobIdTraits<>, SynchronizationPrimitiveType>;
	using JobIdReferenceType = SchedulerType::JobIdReferenceType;

	SchedulerType Scheduler{LogExecutionGraphTest};

	const auto PredecessorStageId = Scheduler.MakeJobIdFromString(TEXTVIEW("PredecessorStage"));
	const auto PredecessorTaskId = Scheduler.MakeJobIdFromString(TEXTVIEW("PredecessorTask"));
	const auto SuccessorStageId = Scheduler.MakeJobIdFromString(TEXTVIEW("SuccessorStage"));
	const auto SuccessorTaskId = Scheduler.MakeJobIdFromString(TEXTVIEW("SuccessorTask"));

	// Enqueue predecessors
	{
		const bool bPredecessorStageEnqueued =
			TestEnqueueStage(Test, Prefix + TEXT("PredecessorStage: "), Scheduler, PredecessorStageId, {});
		ZKZ_RETURN_IF(!bPredecessorStageEnqueued);

		TestCloseStage(Test, Prefix + "PredecessorStage: ", Scheduler, PredecessorStageId);

		auto OptFutureTaskExecution =
			TestEnqueueTask(Test, Prefix + TEXT("PredecessorTask: "), Scheduler, PredecessorTaskId, {});

		TestTaskExecution(Test, Prefix + TEXT("PredecessorTask: "), MoveTemp(*OptFutureTaskExecution));
	}

	// Enqueue successors
	{
		const bool bSuccessorStageEnqueued = TestEnqueueStage(
			Test,
			Prefix + TEXT("SuccessorStage: "),
			Scheduler,
			SuccessorStageId,
			{JobIdReferenceType{PredecessorStageId}, JobIdReferenceType{PredecessorTaskId}});
		ZKZ_RETURN_IF(!bSuccessorStageEnqueued);

		TestCloseStage(Test, Prefix + "SuccessorStage: ", Scheduler, SuccessorStageId);

		auto OptFutureTaskExecution = TestEnqueueTask(
			Test,
			Prefix + TEXT("SuccessorTask: "),
			Scheduler,
			SuccessorTaskId,
			{JobIdReferenceType{PredecessorStageId}, JobIdReferenceType{PredecessorTaskId}});

		TestTaskExecution(Test, Prefix + TEXT("SuccessorTask: "), MoveTemp(*OptFutureTaskExecution));

		TestJobState(
			Test,
			Prefix + TEXT("SuccessorTask: "),
			Scheduler,
			SuccessorTaskId,
			EZkzExecutionGraphJobStateId::Completed);
		TestJobState(
			Test,
			Prefix + TEXT("SuccessorStage: "),
			Scheduler,
			SuccessorStageId,
			EZkzExecutionGraphJobStateId::Completed);
	}
}

template <class SynchronizationPrimitiveType>
void SuccessorsThenPredecessors(FAutomationTestBase& Test, const FString& Prefix)
{
	using SchedulerType =
		TScheduler<decltype(LogExecutionGraphTest), TDefaultSchedulerJobIdTraits<>, SynchronizationPrimitiveType>;
	using JobIdReferenceType = SchedulerType::JobIdReferenceType;

	SchedulerType Scheduler{LogExecutionGraphTest};

	const auto PredecessorStageId = Scheduler.MakeJobIdFromString(TEXTVIEW("PredecessorStage"));
	const auto PredecessorTaskId = Scheduler.MakeJobIdFromString(TEXTVIEW("PredecessorTask"));
	const auto SuccessorStageId = Scheduler.MakeJobIdFromString(TEXTVIEW("SuccessorStage"));
	const auto SuccessorTaskId = Scheduler.MakeJobIdFromString(TEXTVIEW("SuccessorTask"));

	// [1] Enqueue successors
	{
		const bool bSuccessorStageEnqueued = TestEnqueueStage(
			Test,
			Prefix + TEXT("[1] SuccessorStage: "),
			Scheduler,
			SuccessorStageId,
			{JobIdReferenceType{PredecessorStageId}, JobIdReferenceType{PredecessorTaskId}});
		ZKZ_RETURN_IF(!bSuccessorStageEnqueued);

		TestCloseStage(Test, Prefix + "[1] SuccessorStage: ", Scheduler, SuccessorStageId);

		auto OptFutureTaskExecution = TestEnqueueTask(
			Test,
			Prefix + TEXT("[1] SuccessorTask: "),
			Scheduler,
			SuccessorTaskId,
			{JobIdReferenceType{PredecessorStageId}, JobIdReferenceType{PredecessorTaskId}});
		ZKZ_RETURN_IF(!OptFutureTaskExecution.IsSet());

		TestTaskExecution(Test, Prefix + TEXT("[1] SuccessorTask: "), MoveTemp(*OptFutureTaskExecution));

		TestJobState(
			Test,
			Prefix + TEXT("[1] SuccessorTask: "),
			Scheduler,
			SuccessorTaskId,
			EZkzExecutionGraphJobStateId::DefinedTask);
		TestJobState(
			Test,
			Prefix + TEXT("[1] SuccessorStage: "),
			Scheduler,
			SuccessorStageId,
			EZkzExecutionGraphJobStateId::DefinedStage);
		TestJobState(
			Test,
			Prefix + TEXT("[1] PredecessorTask: "),
			Scheduler,
			PredecessorTaskId,
			EZkzExecutionGraphJobStateId::Stub);
		TestJobState(
			Test,
			Prefix + TEXT("[1] PredecessorStage: "),
			Scheduler,
			PredecessorStageId,
			EZkzExecutionGraphJobStateId::Stub);
	}

	TOptional<FJobCompletionPromise> PredecessorTaskCompletionPromise;

	// [2] Enqueue predecessors - don't finish task or close stage
	{
		const bool bPredecessorStageEnqueued =
			TestEnqueueStage(Test, Prefix + TEXT("[2] PredecessorStage: "), Scheduler, PredecessorStageId, {});
		ZKZ_RETURN_IF(!bPredecessorStageEnqueued);

		auto OptFutureTaskExecution =
			TestEnqueueTask(Test, Prefix + TEXT("[2] PredecessorTask: "), Scheduler, PredecessorTaskId, {});
		ZKZ_RETURN_IF(!OptFutureTaskExecution.IsSet());

		TestDelayedTaskExecution(
			Test,
			Prefix + TEXT("[2] PredecessorTask: "),
			MoveTemp(*OptFutureTaskExecution),
			PredecessorTaskCompletionPromise);

		TestJobState(
			Test,
			Prefix + TEXT("[2] SuccessorTask: "),
			Scheduler,
			SuccessorTaskId,
			EZkzExecutionGraphJobStateId::DefinedTask);
		TestJobState(
			Test,
			Prefix + TEXT("[2] SuccessorStage: "),
			Scheduler,
			SuccessorStageId,
			EZkzExecutionGraphJobStateId::DefinedStage);
		TestJobState(
			Test,
			Prefix + TEXT("[2] PredecessorTask: "),
			Scheduler,
			PredecessorTaskId,
			EZkzExecutionGraphJobStateId::ExecutingTask);
		TestJobState(
			Test,
			Prefix + TEXT("[2] PredecessorStage: "),
			Scheduler,
			PredecessorStageId,
			EZkzExecutionGraphJobStateId::ExecutingStage);
	}

	// [3] Complete task - should not yet execute successors
	{
		const bool bTestCompletionPromiseIsSet = Test.TestTrue(
			Prefix + "[3] PredecessorTask: Task completion promise is set", PredecessorTaskCompletionPromise.IsSet());
		ZKZ_RETURN_IF(!bTestCompletionPromiseIsSet);

		PredecessorTaskCompletionPromise->EmplaceValue();

		TestJobState(
			Test,
			Prefix + TEXT("[3] SuccessorTask: "),
			Scheduler,
			SuccessorTaskId,
			EZkzExecutionGraphJobStateId::DefinedTask);
		TestJobState(
			Test,
			Prefix + TEXT("[3] SuccessorStage: "),
			Scheduler,
			SuccessorStageId,
			EZkzExecutionGraphJobStateId::DefinedStage);
		TestJobState(
			Test,
			Prefix + TEXT("[3] PredecessorTask: "),
			Scheduler,
			PredecessorTaskId,
			EZkzExecutionGraphJobStateId::Completed);
		TestJobState(
			Test,
			Prefix + TEXT("[3] PredecessorStage: "),
			Scheduler,
			PredecessorStageId,
			EZkzExecutionGraphJobStateId::ExecutingStage);
	}

	// [4] Close stage - should execute successors
	{
		TestCloseStage(Test, Prefix + "[4] PredecessorStage: ", Scheduler, PredecessorStageId);

		TestJobState(
			Test,
			Prefix + TEXT("[4] SuccessorTask: "),
			Scheduler,
			SuccessorTaskId,
			EZkzExecutionGraphJobStateId::Completed);
		TestJobState(
			Test,
			Prefix + TEXT("[4] SuccessorStage: "),
			Scheduler,
			SuccessorStageId,
			EZkzExecutionGraphJobStateId::Completed);
		TestJobState(
			Test,
			Prefix + TEXT("[4] PredecessorTask: "),
			Scheduler,
			PredecessorTaskId,
			EZkzExecutionGraphJobStateId::Completed);
		TestJobState(
			Test,
			Prefix + TEXT("[4] PredecessorStage: "),
			Scheduler,
			PredecessorStageId,
			EZkzExecutionGraphJobStateId::Completed);
	}
}

template <class SynchronizationPrimitiveType>
void EmptyDefineStageWithPrerequisitesCompletesImmediately(FAutomationTestBase& Test, const FString& Prefix)
{
	using SchedulerType =
		TScheduler<decltype(LogExecutionGraphTest), TDefaultSchedulerJobIdTraits<>, SynchronizationPrimitiveType>;
	using JobIdReferenceType = SchedulerType::JobIdReferenceType;

	SchedulerType Scheduler{LogExecutionGraphTest};

	const auto PredecessorStageId = Scheduler.MakeJobIdFromString(TEXTVIEW("PredecessorStage"));
	const auto PredecessorTaskId = Scheduler.MakeJobIdFromString(TEXTVIEW("PredecessorStage.Task"));
	const auto SuccessorStageId = Scheduler.MakeJobIdFromString(TEXTVIEW("SuccessorStage"));
	const auto FollowingStageId = Scheduler.MakeJobIdFromString(TEXTVIEW("FollowingStage"));

	const bool bPredecessorStageEnqueued =
		TestEnqueueStage(Test, Prefix + TEXT("PredecessorStage: "), Scheduler, PredecessorStageId, {});
	ZKZ_RETURN_IF(!bPredecessorStageEnqueued);
	auto FuturePredecessorTaskExecutionOpt =
		TestEnqueueTask(Test, Prefix + TEXT("PredecessorTask: "), Scheduler, PredecessorTaskId, {});
	ZKZ_RETURN_IF(!FuturePredecessorTaskExecutionOpt.IsSet());

	const bool bSuccessorStageEnqueued = TestEnqueueStage(
		Test, Prefix + TEXT("SuccessorStage: "), Scheduler, SuccessorStageId, {JobIdReferenceType{PredecessorStageId}});
	ZKZ_RETURN_IF(!bSuccessorStageEnqueued);

	const bool bFollowingStageEnqueued = TestEnqueueStage(
		Test, Prefix + TEXT("FollowingStage: "), Scheduler, FollowingStageId, {JobIdReferenceType{SuccessorStageId}});
	ZKZ_RETURN_IF(!bFollowingStageEnqueued);

	TestCloseStage(Test, Prefix + "PredecessorStage: ", Scheduler, PredecessorStageId);
	TestCloseStage(Test, Prefix + "SuccessorStage: ", Scheduler, SuccessorStageId);
	TestCloseStage(Test, Prefix + "FollowingStage: ", Scheduler, FollowingStageId);

	{
		TestJobState(
			Test,
			Prefix + TEXT("PredecessorStage: "),
			Scheduler,
			PredecessorStageId,
			EZkzExecutionGraphJobStateId::ExecutingStage);
		TestJobState(
			Test,
			Prefix + TEXT("SuccessorStage: "),
			Scheduler,
			SuccessorStageId,
			EZkzExecutionGraphJobStateId::DefinedStage);
		TestJobState(
			Test,
			Prefix + TEXT("FollowingStage: "),
			Scheduler,
			FollowingStageId,
			EZkzExecutionGraphJobStateId::DefinedStage);
	}

	TestTaskExecution(Test, Prefix + TEXT("PredecessorTask: "), MoveTemp(*FuturePredecessorTaskExecutionOpt));

	{
		TestJobState(
			Test,
			Prefix + TEXT("PredecessorStage: "),
			Scheduler,
			PredecessorStageId,
			EZkzExecutionGraphJobStateId::Completed);
		TestJobState(
			Test,
			Prefix + TEXT("SuccessorStage: "),
			Scheduler,
			SuccessorStageId,
			EZkzExecutionGraphJobStateId::Completed);
		TestJobState(
			Test,
			Prefix + TEXT("FollowingStage: "),
			Scheduler,
			FollowingStageId,
			EZkzExecutionGraphJobStateId::Completed);
	}
}

template <class SynchronizationPrimitiveType>
void PredecessorsDontHaveSameParent(FAutomationTestBase& Test, const FString& Prefix)
{
	using JobIdTraitsType = TDefaultSchedulerJobIdTraits<>;
	using SchedulerType = TScheduler<decltype(LogExecutionGraphTest), JobIdTraitsType, SynchronizationPrimitiveType>;

	SchedulerType Scheduler{LogExecutionGraphTest};

	const auto Dependency1Id = Scheduler.MakeJobIdFromString(TEXTVIEW("Dependency1"));
	const auto Dependency2Id = Scheduler.MakeJobIdFromString(TEXTVIEW("Parent.Dependency1"));

	auto Result =
		Scheduler.EnqueueTask(Scheduler.MakeJobIdFromString(TEXTVIEW("Task")), {Dependency1Id, Dependency2Id});

	Test.TestTrue(Prefix + TEXT("Has error"), Result.HasError());

	const FError Error = MoveTemp(Result).GetError();
	ZKZ_RETURN_IF(!Test.TestTrue(
		Prefix + TEXT("Error is FPredecessorsDontHaveSameParent"), Error.IsType<FPredecessorsDontHaveSameParent>()));
}

template <class SynchronizationPrimitiveType>
void AddedJobToClosedStage(FAutomationTestBase& Test, const FString& Prefix)
{
	using JobIdTraitsType = TDefaultSchedulerJobIdTraits<>;
	using SchedulerType = TScheduler<decltype(LogExecutionGraphTest), JobIdTraitsType, SynchronizationPrimitiveType>;

	SchedulerType Scheduler{LogExecutionGraphTest};

	const auto StageId = Scheduler.MakeJobIdFromString(TEXTVIEW("Stage"));
	const auto TaskId = Scheduler.MakeJobIdFromString(TEXTVIEW("Stage.Task"));

	TestEnqueueStage(Test, Prefix + "Stage: ", Scheduler, StageId, {});
	TestCloseStage(Test, Prefix + TEXT("Stage: "), Scheduler, StageId);

	auto Result = Scheduler.EnqueueTask(TaskId, {});

	Test.TestTrue(Prefix + TEXT("Has error"), Result.HasError());

	const FError Error = MoveTemp(Result).GetError();
	ZKZ_RETURN_IF(!Test.TestTrue(
		Prefix + TEXT("Error is FAddedJobToClosedStageError"), Error.IsType<FAddedJobToClosedStageError>()));
}

template <class SynchronizationPrimitiveType>
void StageAlreadyClosed(FAutomationTestBase& Test, const FString& Prefix)
{
	using JobIdTraitsType = TDefaultSchedulerJobIdTraits<>;
	using SchedulerType = TScheduler<decltype(LogExecutionGraphTest), JobIdTraitsType, SynchronizationPrimitiveType>;

	SchedulerType Scheduler{LogExecutionGraphTest};

	const auto StageId = Scheduler.MakeJobIdFromString(TEXTVIEW("Stage"));

	TestEnqueueStage(Test, Prefix + "Stage: ", Scheduler, StageId, {});
	TestCloseStage(Test, Prefix + TEXT("Stage: "), Scheduler, StageId);

	auto Result = Scheduler.CloseStage(StageId);

	Test.TestTrue(Prefix + TEXT("Has error"), Result.HasError());

	const FError Error = MoveTemp(Result).GetError();
	ZKZ_RETURN_IF(
		!Test.TestTrue(Prefix + TEXT("Error is FStageAlreadyClosedError"), Error.IsType<FStageAlreadyClosedError>()));
}

template <class SynchronizationPrimitiveType>
void CircularDependency(FAutomationTestBase& Test, const FString& Prefix)
{
	using JobIdTraitsType = TDefaultSchedulerJobIdTraits<>;
	using SchedulerType =
		TScheduler<decltype(LogExecutionGraphTest), JobIdTraitsType, SynchronizationPrimitiveType, TDebugInspections>;
	using JobIdReferenceType = SchedulerType::JobIdReferenceType;

	SchedulerType Scheduler{LogExecutionGraphTest};

	// Stage0 => Task1 => Stage2 => Task3 => Stage0

	const auto Stage0Id = Scheduler.MakeJobIdFromString(TEXTVIEW("Stage0"));
	const auto Task1Id = Scheduler.MakeJobIdFromString(TEXTVIEW("Task1"));
	const auto Stage2Id = Scheduler.MakeJobIdFromString(TEXTVIEW("Stage2"));
	const auto Task3Id = Scheduler.MakeJobIdFromString(TEXTVIEW("Task3"));

	TestEnqueueStage(Test, Prefix + TEXT("Stage0: "), Scheduler, Stage0Id, {JobIdReferenceType{Task1Id}});
	TestEnqueueTask(Test, Prefix + TEXT("Task1: "), Scheduler, Task1Id, {JobIdReferenceType{Stage2Id}});
	TestEnqueueStage(Test, Prefix + TEXT("Stage2: "), Scheduler, Stage2Id, {JobIdReferenceType{Task3Id}});

	auto Result = Scheduler.EnqueueTask(Task3Id, {JobIdReferenceType{Stage0Id}});
	Test.TestTrue(Prefix + TEXT("Task3: Has error when enqueueing circular dependency stage"), Result.HasError());

	FError Error = MoveTemp(Result).GetError();
	const auto* const CircularDependencyError = Error.TryGet<FCircularDependencyError>();
	ZKZ_RETURN_IF(!Test.TestTrue(
		Prefix + TEXT("Task3: Returned error is a circular dependency error"), CircularDependencyError != nullptr));

	Test.TestEqual(
		Prefix + TEXT("Task3: Reported cycle"),
		CircularDependencyError->Cycle,
		{JobIdTraitsType::ToString(Task3Id),
		 JobIdTraitsType::ToString(Stage0Id),
		 JobIdTraitsType::ToString(Task1Id),
		 JobIdTraitsType::ToString(Stage2Id),
		 JobIdTraitsType::ToString(Task3Id)});

	Test.TestEqual(
		Prefix + TEXT("Task3: Error to string"),
		CircularDependencyError->ToString(),
		TEXT("Circular dependency detected: {Task3 => Stage0 => Task1 => Stage2 => Task3}"));
}

// #TODO #Scheduler: I wrote this test because I figured it should be possible to define stubbed jobs even if the
// stage is closed. It doesn't work at the moment, because the stage states don't know about stubs. For that to work,
// the tracked job completion should be added to the stage for stubbed tasks and then retrieved when the task is created.
// This adds complexity and I'm not sure it is needed, so I'll drop it for the time being.
//
// template <class SynchronizationPrimitiveType>
// void CloseStageThenDefineStub(FAutomationTestBase& Test, const FString& Prefix)
// {
// 	using JobIdTraitsType = TDefaultSchedulerJobIdTraits<>;
// 	using SchedulerType =
// 		TScheduler<decltype(LogExecutionGraphTest), JobIdTraitsType, SynchronizationPrimitiveType, TDebugInspections>;
// 	using JobIdReferenceType = SchedulerType::JobIdReferenceType;
//
// 	SchedulerType Scheduler{LogExecutionGraphTest};
//
// 	const auto PredecessorId = Scheduler.MakeJobIdFromString(TEXTVIEW("Stage.Predecessor"));
//
// 	// Defining task introduces predecessor's stub
// 	TestEnqueueTask(
// 		Test,
// 		Prefix + TEXT("Task: "),
// 		Scheduler,
// 		Scheduler.MakeJobIdFromString(TEXTVIEW("Stage.Task")),
// 		{JobIdReferenceType{PredecessorId}});
//
// 	// Stage closed
// 	TestCloseStage(Test, Prefix, Scheduler, Scheduler.MakeJobIdFromString(TEXTVIEW("Stage")));
//
// 	// Defining the predecessor should be fine even after closing if stubbed
// 	TestEnqueueTask(Test, Prefix + TEXT("Predecessor: "), Scheduler, PredecessorId, {});
// }

}  // namespace SchedulerTestPrivate

ZKZ_BEGIN_AUTOMATION_TEST(
	FSchedulerTest,
	"Zakazane.ZakazaneUtilities.ExecutionGraph.Scheduler",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

ZKZ_ADD_TEST(GeneratesUniqueIds_ThreadSafe)
{
	using namespace SchedulerTestPrivate;
	GeneratesUniqueIds<FThreadSafe>(*this, TEXT("Thread safe: "));
}

ZKZ_ADD_TEST(GeneratesUniqueIds_ThreadUnsafe)
{
	using namespace SchedulerTestPrivate;
	GeneratesUniqueIds<FThreadUnsafe>(*this, TEXT("Thread unsafe: "));
}

ZKZ_ADD_TEST(ParentStageThenChildTaskExecuteImmediately_ThreadSafe)
{
	using namespace SchedulerTestPrivate;
	ParentStageThenChildTaskExecuteImmediately<FThreadSafe>(*this, TEXT("Thread safe: "));
}

ZKZ_ADD_TEST(ParentStageThenChildTaskExecuteImmediately_ThreadUnsafe)
{
	using namespace SchedulerTestPrivate;
	ParentStageThenChildTaskExecuteImmediately<FThreadUnsafe>(*this, TEXT("Thread unsafe: "));
}

ZKZ_ADD_TEST(GrandchildTaskThenParentStagesWaitForDefinition_ThreadSafe)
{
	using namespace SchedulerTestPrivate;
	GrandchildTaskThenParentStagesWaitForDefinition<FThreadSafe>(*this, TEXT("Thread safe: "));
}

ZKZ_ADD_TEST(GrandchildTaskThenParentStagesWaitForDefinition_ThreadUnsafe)
{
	using namespace SchedulerTestPrivate;
	GrandchildTaskThenParentStagesWaitForDefinition<FThreadUnsafe>(*this, TEXT("Thread unsafe: "));
}

ZKZ_ADD_TEST(PredecessorsThenSuccessors_ThreadSafe)
{
	using namespace SchedulerTestPrivate;
	PredecessorsThenSuccessors<FThreadSafe>(*this, TEXT("Thread safe: "));
}

ZKZ_ADD_TEST(PredecessorsThenSuccessors_ThreadUnsafe)
{
	using namespace SchedulerTestPrivate;
	PredecessorsThenSuccessors<FThreadUnsafe>(*this, TEXT("Thread unsafe: "));
}

ZKZ_ADD_TEST(SuccessorsThenPredecessors_ThreadSafe)
{
	using namespace SchedulerTestPrivate;
	SuccessorsThenPredecessors<FThreadSafe>(*this, TEXT("Thread safe: "));
}

ZKZ_ADD_TEST(SuccessorsThenPredecessors_ThreadUnsafe)
{
	using namespace SchedulerTestPrivate;
	SuccessorsThenPredecessors<FThreadUnsafe>(*this, TEXT("Thread unsafe: "));
}

ZKZ_ADD_TEST(EmptyDefineStageWithPrerequisitesCompletesImmediately_ThreadSafe)
{
	using namespace SchedulerTestPrivate;
	EmptyDefineStageWithPrerequisitesCompletesImmediately<FThreadSafe>(*this, TEXT("Thread safe: "));
}

ZKZ_ADD_TEST(EmptyDefineStageWithPrerequisitesCompletesImmediately_ThreadUnsafe)
{
	using namespace SchedulerTestPrivate;
	EmptyDefineStageWithPrerequisitesCompletesImmediately<FThreadUnsafe>(*this, TEXT("Thread unsafe: "));
}

ZKZ_ADD_TEST(PredecessorsDontHaveSameParent_ThreadSafe)
{
	using namespace SchedulerTestPrivate;
	PredecessorsDontHaveSameParent<FThreadSafe>(*this, TEXT("Thread safe: "));
}

ZKZ_ADD_TEST(PredecessorsDontHaveSameParent_ThreadUnsafe)
{
	using namespace SchedulerTestPrivate;
	PredecessorsDontHaveSameParent<FThreadUnsafe>(*this, TEXT("Thread unsafe: "));
}

ZKZ_ADD_TEST(AddedJobToClosedStage_ThreadSafe)
{
	using namespace SchedulerTestPrivate;
	AddedJobToClosedStage<FThreadSafe>(*this, TEXT("Thread safe: "));
}

ZKZ_ADD_TEST(AddedJobToClosedStage_ThreadUnsafe)
{
	using namespace SchedulerTestPrivate;
	AddedJobToClosedStage<FThreadUnsafe>(*this, TEXT("Thread unsafe: "));
}

ZKZ_ADD_TEST(StageAlreadyClosed_ThreadSafe)
{
	using namespace SchedulerTestPrivate;
	StageAlreadyClosed<FThreadSafe>(*this, TEXT("Thread safe: "));
}

ZKZ_ADD_TEST(StageAlreadyClosed_ThreadUnsafe)
{
	using namespace SchedulerTestPrivate;
	StageAlreadyClosed<FThreadUnsafe>(*this, TEXT("Thread unsafe: "));
}

ZKZ_ADD_TEST(CircularDependency_ThreadSafe)
{
	using namespace SchedulerTestPrivate;
	CircularDependency<FThreadSafe>(*this, TEXT("Thread safe: "));
}

ZKZ_ADD_TEST(CircularDependency_ThreadUnsafe)
{
	using namespace SchedulerTestPrivate;
	CircularDependency<FThreadUnsafe>(*this, TEXT("Thread unsafe: "));
}

ZKZ_END_AUTOMATION_TEST(FSchedulerTest)

}  // namespace Zkz::ExecutionGraph::Test
