import { CommonModule } from "@angular/common";
import { NgModule } from "@angular/core";
import { FormsModule } from "@angular/forms";
import { ManagementConsoleComponent } from "./components/console/console.component";
import { RegisterDeviceComponent } from "./components/register-device/register-device.component";
import { RegisterComponent } from "./components/register/register.component";
import { SearchDevicesComponent } from "./components/search-devices/search-devices.component";
import { ManagementRoutingModule } from "./management-routing.module";

@NgModule({
  declarations: [
    ManagementConsoleComponent,
    SearchDevicesComponent,
    RegisterDeviceComponent,
    RegisterComponent,
  ],
  imports: [CommonModule, FormsModule, ManagementRoutingModule],
})
export class ManagementModule {}
