package net.minecraft.src;

import java.util.ArrayList;
import java.util.Iterator;

public class EntityAITasks {
	private ArrayList tasksToDo = new ArrayList();
	private ArrayList executingTasks = new ArrayList();

	public void addTask(int var1, EntityAIBase var2) {
		this.tasksToDo.add(new EntityAITaskEntry(this, var1, var2));
	}

	public void onUpdateTasks() {
		ArrayList var1 = new ArrayList();
		Iterator var2 = this.tasksToDo.iterator();

		while(true) {
			EntityAITaskEntry var3;
			while(true) {
				if(!var2.hasNext()) {
					boolean var5 = false;
					if(var5 && var1.size() > 0) {
						System.out.println("Starting: ");
					}

					Iterator var6;
					EntityAITaskEntry var7;
					for(var6 = var1.iterator(); var6.hasNext(); var7.action.startExecuting()) {
						var7 = (EntityAITaskEntry)var6.next();
						if(var5) {
							System.out.println(var7.action.toString() + ", ");
						}
					}

					if(var5 && this.executingTasks.size() > 0) {
						System.out.println("Running: ");
					}

					for(var6 = this.executingTasks.iterator(); var6.hasNext(); var7.action.updateTask()) {
						var7 = (EntityAITaskEntry)var6.next();
						if(var5) {
							System.out.println(var7.action.toString());
						}
					}

					return;
				}

				var3 = (EntityAITaskEntry)var2.next();
				boolean var4 = this.executingTasks.contains(var3);
				if(!var4) {
					break;
				}

				if(!this.func_46116_a(var3) || !var3.action.continueExecuting()) {
					var3.action.resetTask();
					this.executingTasks.remove(var3);
					break;
				}
			}

			if(this.func_46116_a(var3) && var3.action.shouldExecute()) {
				var1.add(var3);
				this.executingTasks.add(var3);
			}
		}
	}

	private boolean func_46116_a(EntityAITaskEntry var1) {
		Iterator var2 = this.tasksToDo.iterator();

		while(var2.hasNext()) {
			EntityAITaskEntry var3 = (EntityAITaskEntry)var2.next();
			if(var3 != var1) {
				if(var1.priority >= var3.priority) {
					if(this.executingTasks.contains(var3) && !this.areTasksCompatible(var1, var3)) {
						return false;
					}
				} else if(this.executingTasks.contains(var3) && !var3.action.isContinuous()) {
					return false;
				}
			}
		}

		return true;
	}

	private boolean areTasksCompatible(EntityAITaskEntry var1, EntityAITaskEntry var2) {
		return (var1.action.getMutexBits() & var2.action.getMutexBits()) == 0;
	}
}
