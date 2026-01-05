#include "Zakazane/StagedExecution/Scheduler.h"
#include "Zakazane/Test/Test.h"

namespace Zkz::StagedExecution::Test
{

struct FTestTask
{
	void Enqueue(
		TScheduler<FName>& Scheduler, const FName StageId, const FName TaskId, FAutomationTestBase* Test = nullptr)
	{
		TAddTaskToStageResult<FName> AddTaskToStageResult = Scheduler.AddTaskToStage(StageId, TaskId, GLog);
		ZKZ_RETURN_IF(
			Test != nullptr ? !Test->TestTrue("Task added", AddTaskToStageResult.HasValue())
							: !AddTaskToStageResult.HasValue());

		FFutureTaskExecution FutureTaskExecution = MoveTemp(AddTaskToStageResult).GetValue();
		IfNotCanceled(
			MoveTemp(FutureTaskExecution),
			[this](FTaskCompletionPromise TaskCompletionPromise) { return Run(MoveTemp(TaskCompletionPromise)); });
	}

	void Run(FTaskCompletionPromise InCompletionPromise)
	{
		bHasExecuted = true;
		CompletionPromise = MoveTemp(InCompletionPromise);
	}

	void Finish()
	{
		CompletionPromise.EmplaceValue();
	}

	bool bHasExecuted = false;
	FTaskCompletionPromise CompletionPromise;
};

ZKZ_BEGIN_AUTOMATION_TEST(
	FStagedExecutionTest,
	"Zakazane.ZakazaneUtilities.ExecutionOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

ZKZ_ADD_TEST(BasicUsage)
{
	TScheduler<FName> Scheduler;

	// To make an apple pie you need flour and apples (cooking)
	// To have flour you need to mill wheat (cooking -> milling)
	// To have apples you need to collect them. This is performed by cheap seasonal laborers (cooking -> cheap labor)
	// Both wheat and apples need time to grow (milling, cheap labor -> farming)
	// To plant them you need seeds (farming -> farming prep)

	FTestTask BuySeeds;
	BuySeeds.Enqueue(Scheduler, "Farming prep", "Buy seeds", this);
	Scheduler.SetAllTasksAdded("Farming prep", GLog);
	TestTrue("Dependency ok", Scheduler.AddStage("Farming prep", {}, GLog).HasValue());

	FTestTask GrowWheat;
	FTestTask GrowAppleTrees;
	GrowWheat.Enqueue(Scheduler, "Farming", "Grow wheat", this);
	GrowAppleTrees.Enqueue(Scheduler, "Farming", "Grow apple trees", this);
	Scheduler.SetAllTasksAdded("Farming", GLog);
	TestTrue("Dependency ok", Scheduler.AddStage("Farming", {"Farming prep"}, GLog).HasValue());

	FTestTask MakeFlour;
	MakeFlour.Enqueue(Scheduler, "Milling", "Make flour", this);
	Scheduler.SetAllTasksAdded("Milling", GLog);
	TestTrue("Dependency ok", Scheduler.AddStage("Milling", {"Farming"}, GLog).HasValue());

	FTestTask CollectApples;
	CollectApples.Enqueue(Scheduler, "Cheap labor", "Collect apples", this);
	Scheduler.SetAllTasksAdded("Cheap labor", GLog);
	TestTrue("Dependency ok", Scheduler.AddStage("Cheap labor", {"Farming"}, GLog).HasValue());

	FTestTask MakeApplePie;
	MakeApplePie.Enqueue(Scheduler, "Cooking", "Make apple pie", this);
	Scheduler.SetAllTasksAdded("Cooking", GLog);
	TestTrue("Dependency ok", Scheduler.AddStage("Cooking", {"Milling", "Cheap labor"}, GLog).HasValue());

	TestTrue("1. BuySeeds executed", BuySeeds.bHasExecuted);
	TestFalse("1. GrowWheat not executed", GrowWheat.bHasExecuted);
	TestFalse("1. GrowAppleTrees not executed", GrowAppleTrees.bHasExecuted);
	TestFalse("1. MakeFlour not executed", MakeFlour.bHasExecuted);
	TestFalse("1. CollectApples not executed", CollectApples.bHasExecuted);
	TestFalse("1. MakeApplePie not executed", MakeApplePie.bHasExecuted);

	BuySeeds.Finish();

	TestTrue("2. GrowWheat executed", GrowWheat.bHasExecuted);
	TestTrue("2. GrowAppleTrees executed", GrowAppleTrees.bHasExecuted);
	TestFalse("2. MakeFlour not executed", MakeFlour.bHasExecuted);
	TestFalse("2. CollectApples not executed", CollectApples.bHasExecuted);
	TestFalse("2. MakeApplePie not executed", MakeApplePie.bHasExecuted);

	GrowWheat.Finish();

	TestFalse("3. MakeFlour not executed", MakeFlour.bHasExecuted);
	TestFalse("3. CollectApples not executed", CollectApples.bHasExecuted);
	TestFalse("3. MakeApplePie not executed", MakeApplePie.bHasExecuted);

	GrowAppleTrees.Finish();

	TestTrue("4. MakeFlour executed", MakeFlour.bHasExecuted);
	TestTrue("4. CollectApples executed", CollectApples.bHasExecuted);
	TestFalse("4. MakeApplePie not executed", MakeApplePie.bHasExecuted);

	MakeFlour.Finish();
	CollectApples.Finish();

	TestTrue("5. MakeApplePie executed", MakeApplePie.bHasExecuted);

	MakeApplePie.Finish();
}

ZKZ_ADD_TEST(CanDependOnAndCollectTasksForUndefinedStage)
{
	TScheduler<FName> Scheduler;

	FTestTask BTask;
	BTask.Enqueue(Scheduler, "B", "Task", this);
	Scheduler.SetAllTasksAdded("B", GLog);

	TestTrue("Dependency ok", Scheduler.AddStage("B", {"A"}, GLog).HasValue());

	TestFalse("Task not executed while prerequisite stage not complete", BTask.bHasExecuted);

	Scheduler.SetAllTasksAdded("A", GLog);
	TestFalse("Task not executed while prerequisite stage not defined", BTask.bHasExecuted);

	TestTrue("Dependency ok", Scheduler.AddStage("A", {}, GLog).HasValue());

	TestTrue("Task executed when prerequisite stage complete", BTask.bHasExecuted);

	BTask.Finish();
}

ZKZ_ADD_TEST(PhaseDependencyCycleReturnsError)
{
	if constexpr (!GPerformInspections)
	{
		TestTrue("Circular dependencies not checked with disabled sanity checks", true);
		return;
	}

	TScheduler<FName> Scheduler;

	// A -> B -> C -> A. Additional prerequisites D and E are also added for testing.

	TestTrue("D -> E fine", Scheduler.AddStage("D", {"E"}).HasValue());
	TestTrue("E -> 0 fine", Scheduler.AddStage("E", {}).HasValue());

	TestTrue("A -> B fine", Scheduler.AddStage("A", {"D", "B", "E"}).HasValue());
	TestTrue("C -> A fine", Scheduler.AddStage("C", {"D", "A", "E"}).HasValue());

	const TScheduler<FName>::FAddStageResult Result = Scheduler.AddStage("B", {"D", "C", "E"});
	ZKZ_RETURN_IF(!TestTrue("B -> C returns error", Result.HasError()));

	TestEqual(
		"B -> C returns error with correct string",
		ToString(Result.GetError()),
		R"(Adding stage "B" with prerequisite(s) {"D", "C", "E"} would introduce cycle "B" -> "C" -> "A" -> "B". Aborting operation.)");
	const auto* const CircularDependencyError = Result.GetError().TryGet<TStageCircularDependencyError<FName>>();
	ZKZ_RETURN_IF(!TestTrue("B -> C returns circular dependency error", CircularDependencyError != nullptr));

	TestEqual("B -> C circular dependency error contains cycle", CircularDependencyError->Cycle, {"B", "C", "A", "B"});
}

ZKZ_ADD_TEST(RedefiningPhaseDependenciesReturnsError)
{
	TScheduler<FName> Scheduler;
	TestTrue("A -> B fine", Scheduler.AddStage("A", {"B"}).HasValue());

	const TScheduler<FName>::FAddStageResult Result = Scheduler.AddStage("A", {"B"});
	TestTrue("A -> B duplicate returns error", Result.HasError());

	TestEqual(
		"A -> B duplicate returns error with correct string",
		ToString(Result.GetError()),
		R"(Stage "A" has already been added. Aborting operation.)");

	const auto* const StageAlreadyAddedError = Result.GetError().TryGet<TStageAlreadyAddedError<FName>>();
	ZKZ_RETURN_IF(!TestTrue("A -> B returns circular dependency error", StageAlreadyAddedError != nullptr));

	TestEqual("A -> B circular dependency error contains stage id", StageAlreadyAddedError->StageId, FName{"A"});
}

ZKZ_ADD_TEST(TaskAddedAfterAllTasksCollectedReturnsError)
{
	TScheduler<FName> Scheduler;
	Scheduler.SetAllTasksAdded("A", GLog);

	const TScheduler<FName>::FAddTaskToStageResult AddTaskToStageResult = Scheduler.AddTaskToStage("A", "Task", GLog);
	TestTrue("Adding task to all tasks collected stage returns error", AddTaskToStageResult.HasError());
}

ZKZ_ADD_TEST(SimpleAddTask)
{
	TScheduler<FName> Scheduler;
	TAddTaskResult<FName> AddTaskResultA = Scheduler.AddTask("A", {});
	ZKZ_RETURN_IF(!TestTrue("Add task A", AddTaskResultA.HasValue()));
	FFutureTaskExecution FutureTaskExecutionA = MoveTemp(AddTaskResultA).GetValue();

	TestTrue("A ready immediately", FutureTaskExecutionA.IsReady());

	TAddTaskResult<FName> AddTaskResultB = Scheduler.AddTask("B", {"A"});
	TestTrue("Add task B", AddTaskResultB.HasValue());

	TestTrue("B not ready", !AddTaskResultB.GetValue().IsReady());

	FTaskCompletionPromise TaskCompletionPromiseA = FutureTaskExecutionA.Consume().GetValue();
	TaskCompletionPromiseA.EmplaceValue();

	TestTrue("B ready", AddTaskResultB.GetValue().IsReady());

	TAddTaskResult<FName> ReAddTaskResultA = Scheduler.AddTask("A", {});
	TestTrue("Re-add task A returns error", ReAddTaskResultA.HasError());
}

ZKZ_ADD_TEST(CanAddTaskBeforeStageDefined)
{
	TScheduler<FName> Scheduler;

	FTestTask Task;
	Task.Enqueue(Scheduler, "A", "Task", this);

	ZKZ_RETURN_IF(TestTrue("Add stage A", Scheduler.AddStage("A", {}).HasValue()));

	TestFalse("Task not run", Task.bHasExecuted);
	Scheduler.SetAllTasksAdded("A");

	TestTrue("Task run", Task.bHasExecuted);
}

ZKZ_ADD_TEST(CanSetAllTAsksAddedBeforeStageDefined)
{
	TScheduler<FName> Scheduler;

	FTestTask Task;
	Task.Enqueue(Scheduler, "A", "Task", this);
	Scheduler.SetAllTasksAdded("A");

	TestFalse("Task not run", Task.bHasExecuted);

	ZKZ_RETURN_IF(TestTrue("Add stage A", Scheduler.AddStage("A", {}).HasValue()));

	TestTrue("Task run", Task.bHasExecuted);
}

ZKZ_END_AUTOMATION_TEST(FStagedExecutionTest);

}  // namespace Zkz::StagedExecution::Test
