import { NgModule } from '@angular/core';
import { RouterModule, Routes } from '@angular/router';
import { ManagementConsoleComponent } from './components/console/console.component';
import { RegisterComponent } from './components/register/register.component';

const routes: Routes = [
	{
		path: 'console',
		component: ManagementConsoleComponent
	},
	{
		path: 'register',
		component: RegisterComponent
	}
];

@NgModule({
	imports: [RouterModule.forChild(routes)],
	exports: [RouterModule]
})
export class ManagementRoutingModule { }
