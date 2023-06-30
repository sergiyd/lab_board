import { NgModule } from "@angular/core";
import { Routes, RouterModule } from "@angular/router";

import { LiveComponent } from "./components/live/live.component";
import { FilesComponent } from "./components/files/files.component";
import { PlayFileComponent } from "./components/play-file/play-file.component";

const routes: Routes = [
  {
    path: "live",
    component: LiveComponent,
  },
  {
    path: "files",
    component: FilesComponent,
  },
  {
    path: "play-file/:name",
    component: PlayFileComponent,
  },
];

@NgModule({
  imports: [RouterModule.forChild(routes)],
  exports: [RouterModule],
})
export class DataRoutingModule {}
