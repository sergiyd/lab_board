import { NgModule } from "@angular/core";
import { Routes, RouterModule } from "@angular/router";

const routes: Routes = [
  {
    path: "management",
    loadChildren: () =>
      import("./management/management.module").then(
        (mod) => mod.ManagementModule
      ),
  },
  {
    path: "data",
    loadChildren: () =>
      import("./data/data.module").then((mod) => mod.DataModule),
  },
  {
    path: "",
    redirectTo: "",
    pathMatch: "full",
  },
];

@NgModule({
  imports: [RouterModule.forRoot(routes)],
  exports: [RouterModule],
})
export class AppRoutingModule {}
