//////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2013-2014, Image Engine Design Inc. All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are
//  met:
//
//      * Redistributions of source code must retain the above
//        copyright notice, this list of conditions and the following
//        disclaimer.
//
//      * Redistributions in binary form must reproduce the above
//        copyright notice, this list of conditions and the following
//        disclaimer in the documentation and/or other materials provided with
//        the distribution.
//
//      * Neither the name of John Haddon nor the names of
//        any other contributors to this software may be used to endorse or
//        promote products derived from this software without specific prior
//        written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
//  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
//  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
//  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
//  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
//  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//////////////////////////////////////////////////////////////////////////

#ifndef GAFFERDISPATCH_EXECUTABLENODE_H
#define GAFFERDISPATCH_EXECUTABLENODE_H

#include "IECore/MurmurHash.h"

#include "Gaffer/Node.h"
#include "Gaffer/Plug.h"

#include "GafferDispatch/TypeIds.h"

namespace Gaffer
{

IE_CORE_FORWARDDECLARE( Context )
IE_CORE_FORWARDDECLARE( ArrayPlug )

} // namespace Gaffer

namespace GafferDispatch
{

IE_CORE_FORWARDDECLARE( ExecutableNode )

/// A base class for nodes with external side effects such as the creation of files, rendering, etc.
/// ExecutableNodes can be chained together with other ExecutableNodes to define a required execution
/// order. Typically ExecutableNodes should be executed by Dispatcher classes that can query the
/// required execution order and schedule Tasks appropriately.
class ExecutableNode : public Gaffer::Node
{

	public :

		IE_CORE_FORWARDDECLARE( TaskPlug )

		/// Defines a task for dispatch by storing a TaskPlug and
		/// the context in which it should be executed. This is
		/// primarily for the use of Dispatcher classes. See TaskPlug
		/// for the main public interface for the execution of
		/// individual tasks.
		class Task
		{
			public :

				/// Constructs a task representing a call to
				/// `plug->execute()` in the specified context.
				/// A copy of the context is stored.
				Task( ConstTaskPlugPtr plug, const Gaffer::Context *context );
				Task( const Task &t );
				/// Returns the TaskPlug component of the task.
				const TaskPlug *plug() const;
				/// Returns the Context component of the task.
				const Gaffer::Context *context() const;
				/// Returns a hash uniquely representing the side effects
				/// of the task. This is stored from `plug->hash()`
				/// during construction, so editing the node or upstream
				/// graph will invalidate the hash (and therefore the task).
				const IECore::MurmurHash hash() const;
				/// Compares hashes.
				bool operator == ( const Task &rhs ) const;
				/// Compares hashes.
				bool operator < ( const Task &rhs ) const;

				/// \deprecated
				Task( ExecutableNodePtr n, const Gaffer::Context *c );
				/// \deprecated
				const ExecutableNode *node() const;

			private :

				ConstTaskPlugPtr m_plug;
				Gaffer::ConstContextPtr m_context;
				IECore::MurmurHash m_hash;

		};

		typedef std::vector<Task> Tasks;
		/// \todo This is unused - remove it.
		typedef std::vector<Gaffer::ConstContextPtr> Contexts;

		IE_CORE_DECLARERUNTIMETYPEDEXTENSION( GafferDispatch::ExecutableNode, ExecutableNodeTypeId, Gaffer::Node );

		ExecutableNode( const std::string &name=defaultName<ExecutableNode>() );
		virtual ~ExecutableNode();

		/// Plug type used to represent tasks within the
		/// node graph. This provides the primary public
		/// interface for querying and executing tasks.
		class TaskPlug : public Gaffer::Plug
		{

			public :

				IE_CORE_DECLARERUNTIMETYPEDEXTENSION( GafferDispatch::ExecutableNode::TaskPlug, ExecutableNodeTaskPlugTypeId, Gaffer::Plug );

				TaskPlug( const std::string &name=defaultName<TaskPlug>(), Direction direction=In, unsigned flags=Default );

				virtual bool acceptsChild( const Gaffer::GraphComponent *potentialChild ) const;
				virtual bool acceptsInput( const Gaffer::Plug *input ) const;
				virtual Gaffer::PlugPtr createCounterpart( const std::string &name, Direction direction ) const;

				/// Returns a hash representing the side effects of
				/// calling `execute()` in the current context.
				IECore::MurmurHash hash() const;
				/// Executes the task for the current context.
				void execute() const;
				/// Executes a sequence of tasks by taking the current context
				/// and varying it over the sequence of frames. This should be
				/// preferred over `execute()` if `requiresSequenceExecution()`
				/// returns true.
				void executeSequence( const std::vector<float> &frames ) const;
				/// Returns true if multiple frame execution must be done
				/// via a single call to `executeSequence()`, and shouldn't
				/// be split into several distinct calls.
				bool requiresSequenceExecution() const;

				/// Fills tasks with all Tasks that must be completed before `execute()`
				/// is called in the current context. Primarily for use by the Dispatcher
				/// class.
				void preTasks( Tasks &tasks ) const;
				/// Fills tasks with Tasks that must be executed following the execution
				/// of this node in the current context. Primarily for use by the Dispatcher
				/// class.
				void postTasks( Tasks &tasks ) const;

		};

		typedef Gaffer::FilteredChildIterator<Gaffer::PlugPredicate<Gaffer::Plug::Invalid, TaskPlug> > TaskPlugIterator;

		/// Input plugs to which upstream tasks may be connected to cause them
		/// to be executed before this node.
		Gaffer::ArrayPlug *preTasksPlug();
		const Gaffer::ArrayPlug *preTasksPlug() const;

		/// Input plugs to which tasks may be connected to cause them to be executed
		/// after this node, potentially in parallel with downstream tasks.
		Gaffer::ArrayPlug *postTasksPlug();
		const Gaffer::ArrayPlug *postTasksPlug() const;

		/// Output plug which can be connected to downstream preTasks plugs to cause
		/// this node to be executed before the downstream nodes.
		TaskPlug *taskPlug();
		const TaskPlug *taskPlug() const;

		/// Parent plug used by Dispatchers to expose per-node dispatcher settings.
		/// See the "ExecutableNode Customization" section of the Gaffer::Dispatcher
		/// documentation for more details.
		Gaffer::Plug *dispatcherPlug();
		const Gaffer::Plug *dispatcherPlug() const;

	/// All methods below here are considered to be protected, and will become
	/// so in a future release. Use the TaskPlug accessor methods instead of
	/// calling them directly.
	/// \todo Add protected access specifier.

		/// Called by `TaskPlug::preTasks()`. The default implementation collects
		/// the upstream Tasks connected into the preTasksPlug().
		/// \todo Add `const TaskPlug *plug` argument, to allow ExecutableNodes to
		/// have multiple output tasks should they so desire.
		virtual void preTasks( const Gaffer::Context *context, Tasks &tasks ) const;

		/// Called by `TaskPlug::postTasks()`. The default implementation collects
		/// the tasks connected into the postTasksPlug().
		/// \todo Add `const TaskPlug *plug` argument.
		virtual void postTasks( const Gaffer::Context *context, Tasks &tasks ) const;

		/// Called by `TaskPlug::hash()`. Derived nodes should first call the base
		/// implementation and append to the returned hash. Nodes can indicate that they
		/// don't cause side effects for the given context by returning a default hash.
		/// \todo Add `const TaskPlug *plug` argument.
		virtual IECore::MurmurHash hash( const Gaffer::Context *context ) const = 0;

		/// Called by `TaskPlug::execute()`.
		/// \todo Add `const TaskPlug *plug, const Context *context` arguments,
		/// to allow ExecutableNodes to have multiple output tasks should
		/// they so desire.
		virtual void execute() const = 0;

		/// Called by `TaskPlug::executeSequence()`.
		/// \todo Add `const TaskPlug *plug, const Context *context` arguments.
		virtual void executeSequence( const std::vector<float> &frames ) const;

		/// Called by `TaskPlug::requiresSequenceExecution()`.
		/// The default implementation returns false.
		/// \todo Add `const TaskPlug *plug, const Context *context` arguments.
		virtual bool requiresSequenceExecution() const;

	private :

		static size_t g_firstPlugIndex;

};

} // namespace GafferDispatch

#endif // GAFFERDISPATCH_EXECUTABLENODE_H
