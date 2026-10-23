// SPDX-License-Identifier: GPL-2.0-or-later
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Security.Cryptography;
using System.Security.Principal;
using System.Text;
using System.Web.Script.Serialization;
using System.Windows.Forms;
using System.Globalization;
using System.Runtime.InteropServices;

internal sealed class CoreContract {
 public int Schema,Abi; public string Kind,Version,Consumer,MinVersion,MaxVersionExclusive;
}
internal sealed class SharedRecord {
 public List<CoreContract> Requirements=new List<CoreContract>();
 public int Abi; public string Version,Hash; public bool OwnedByTitle; public List<string> Consumers=new List<string>();
}
internal sealed class InstallRecord { public string Version,PluginHash; }
internal static class Setup {
 const string Consumer="obs-dynamic-stream-title";
 const string Plugin="obs-plugins/64bit/obs-dynamic-stream-title.dll";
 const string Core="bin/64bit/nexus-twitch-core.dll";
 const string Manifest="data/obs-plugins/obs-dynamic-stream-title/install.json";
 const string Shared="data/nexus-twitch-core/runtime.json";
 static readonly JavaScriptSerializer Json=new JavaScriptSerializer();
 static string Language=CultureInfo.CurrentUICulture.TwoLetterISOLanguageName;
 static Dictionary<string,Dictionary<string,string>> Translations;
 static bool FixtureMode;
 const string LegacyOverlayHash="24177383b0de5a38e8d1619e21ce80a48e2455cdda28c5f31f646dfaeb85dc5a";
 const string PreVersionOverlayHash="630501e842c70acf969239b96e7431f3861c44d5edcc6a3f68b2b9ddfc015a47";
 const string PreVersionCoreHash="c49a76418bb42db5b6a8f4e40892f5d8e2be4e38ae3019b86dd33f2078c52469";
 [DllImport("kernel32",CharSet=CharSet.Unicode,SetLastError=true)]static extern IntPtr LoadLibraryEx(string path,IntPtr file,uint flags);
 [DllImport("kernel32",CharSet=CharSet.Unicode)]static extern IntPtr FindResource(IntPtr module,string name,IntPtr type);
 [DllImport("kernel32")]static extern uint SizeofResource(IntPtr module,IntPtr resource);
 [DllImport("kernel32")]static extern IntPtr LoadResource(IntPtr module,IntPtr resource);
 [DllImport("kernel32")]static extern IntPtr LockResource(IntPtr resource);
 [DllImport("kernel32")]static extern bool FreeLibrary(IntPtr module);
 static Version SemVer(string text){
  Version version;if(text==null||!System.Text.RegularExpressions.Regex.IsMatch(text,@"^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$")||!Version.TryParse(text,out version))throw new IOException(T("Contrato de Twitch Core inválido."));return version;
 }
 static CoreContract ValidateContract(CoreContract contract,string kind){
  if(contract==null||contract.Schema!=1||contract.Kind!=kind||contract.Abi<=0)throw new IOException(T("Contrato de Twitch Core inválido."));
  if(kind=="core")SemVer(contract.Version);
  else if(String.IsNullOrWhiteSpace(contract.Consumer)||SemVer(contract.MinVersion)>=SemVer(contract.MaxVersionExclusive))throw new IOException(T("Contrato de Twitch Core inválido."));
  return contract;
 }
 static CoreContract DllContract(string path){
  // DATAFILE | IMAGE_RESOURCE: inspect metadata without running DllMain or loading dependencies.
  IntPtr module=LoadLibraryEx(Path.GetFullPath(path),IntPtr.Zero,0x22);if(module==IntPtr.Zero)return null;
  try{IntPtr resource=FindResource(module,"NEXUS_TWITCH_CONTRACT",new IntPtr(10));if(resource==IntPtr.Zero)return null;
   uint count=SizeofResource(module,resource);if(count==0||count>65536)throw new IOException(T("Contrato de Twitch Core inválido."));
   IntPtr bytes=LockResource(LoadResource(module,resource));if(bytes==IntPtr.Zero)throw new IOException(T("Contrato de Twitch Core inválido."));
   byte[] data=new byte[count];Marshal.Copy(bytes,data,0,(int)count);return Json.Deserialize<CoreContract>(Encoding.UTF8.GetString(data));
  }finally{FreeLibrary(module);}
 }
 static CoreContract PackedContract(string name,string kind){return ValidateContract(Json.Deserialize<CoreContract>(Encoding.UTF8.GetString(Resource(name))),kind);}
 static bool Accepts(CoreContract consumer,CoreContract core){
  ValidateContract(consumer,"consumer");ValidateContract(core,"core");
  return consumer.Abi==core.Abi&&SemVer(core.Version)>=SemVer(consumer.MinVersion)&&SemVer(core.Version)<SemVer(consumer.MaxVersionExclusive);
 }
 static void RequireCompatible(CoreContract consumer,CoreContract core){
  if(!Accepts(consumer,core))throw new IOException(String.Format(T("{0} requiere Twitch Core >= {1}, < {2}, ABI {3}. Core seleccionado: {4}, ABI {5}."),consumer.Consumer,consumer.MinVersion,consumer.MaxVersionExclusive,consumer.Abi,core.Version,core.Abi));
 }
 static string SupportedLegacyHash {get{return LegacyOverlayHash.Replace(" ","");}}
 static void PlanOverlayUpgrade(string path,Dictionary<string,byte[]> writes){
  string hash=FileHash(path);if(hash==Payload.OverlayHash)return;
  var declared=DllContract(path);
  if(declared!=null){ValidateContract(declared,"consumer");if(declared.Consumer!="obs-twitch-overlays")throw new IOException(T("Contrato de Twitch Core inválido."));return;}
  if(hash!=SupportedLegacyHash&&hash!=PreVersionOverlayHash)throw new IOException(T("Overlay instalado requiere una actualización compatible antes de compartir este Core. No se modificó Overlay."));
  // Validate the actual DLL directory, including global plugin paths, before replacement.
  path=PathIn(Path.GetDirectoryName(path),Path.GetFileName(path));
  byte[] bytes=Resource("overlay");if(Hash(bytes)!=Payload.OverlayHash)throw new IOException(T("La integridad del paquete no es correcta."));
  writes[path]=bytes;
  string pluginRoot=Path.GetFullPath(Path.Combine(Path.GetDirectoryName(path),"..",".."));
  string manifest=PathIn(pluginRoot,"setup/installed.tsv");
  if(File.Exists(manifest)){
   string text=File.ReadAllText(manifest);string[] lines=text.Split(new[]{'\n'});
   for(int i=0;i<lines.Length;i++){string[] fields=lines[i].TrimEnd('\r').Split('\t');if(fields.Length==2&&fields[1].Replace('\\','/').Equals("bin/64bit/obs-twitch-overlays.dll",StringComparison.OrdinalIgnoreCase)){
    if(!fields[0].Equals(hash,StringComparison.OrdinalIgnoreCase))throw new IOException(T("Manifiesto inválido."));
    lines[i]=Payload.OverlayHash.ToUpperInvariant()+"\t"+fields[1];
   }}writes[manifest]=Encoding.UTF8.GetBytes(String.Join("\n",lines));
  }
 }
 static string T(string source){
  if(Translations==null)Translations=Json.Deserialize<Dictionary<string,Dictionary<string,string>>>(Encoding.UTF8.GetString(Resource("translations")));
  Dictionary<string,string> row;string value;return Translations.TryGetValue(source,out row)&&row.TryGetValue(Language,out value)?value:Translations.TryGetValue(source,out row)&&row.TryGetValue("en",out value)?value:source;
 }
 static string PathIn(string root,string relative){
  root=Path.GetFullPath(root).TrimEnd(Path.DirectorySeparatorChar)+Path.DirectorySeparatorChar;
  string path=Path.GetFullPath(Path.Combine(root,relative.Replace('/',Path.DirectorySeparatorChar)));
  if(!path.StartsWith(root,StringComparison.OrdinalIgnoreCase))throw new IOException(T("Ruta fuera de OBS."));
  for(string p=Path.GetDirectoryName(path);p!=null;p=Path.GetDirectoryName(p))if(Directory.Exists(p)&&(File.GetAttributes(p)&FileAttributes.ReparsePoint)!=0)throw new IOException(T("No se admiten enlaces de directorios."));
  return path;
 }
 static byte[] Resource(string name){using(Stream s=Assembly.GetExecutingAssembly().GetManifestResourceStream(name)){if(s==null)throw new IOException(T("Recurso ausente: ")+name);using(var m=new MemoryStream()){s.CopyTo(m);return m.ToArray();}}}
 static string Hash(byte[] bytes){using(var sha=SHA256.Create())return BitConverter.ToString(sha.ComputeHash(bytes)).Replace("-","").ToLowerInvariant();}
 static string FileHash(string path){return File.Exists(path)?Hash(File.ReadAllBytes(path)):null;}
 static TRecord Read<TRecord>(string path) where TRecord:class {if(!File.Exists(path))return null;if(new FileInfo(path).Length>65536)throw new IOException(T("Manifiesto demasiado grande."));var row=Json.Deserialize<TRecord>(File.ReadAllText(path));if(row==null)throw new IOException(T("Manifiesto inválido."));return row;}
 static byte[] Encode(object row){return Encoding.UTF8.GetBytes(Json.Serialize(row));}
 static void Atomic(string path,byte[] bytes,bool backup){
  Directory.CreateDirectory(Path.GetDirectoryName(path));string temp=path+".dst-new-"+Guid.NewGuid().ToString("N");
  try{File.WriteAllBytes(temp,bytes);if(File.Exists(path)){string copy=backup?path+".dst-backup-"+DateTime.UtcNow.ToString("yyyyMMddHHmmssfff")+"-"+Guid.NewGuid().ToString("N"):null;File.Replace(temp,path,copy);}else File.Move(temp,path);}finally{if(File.Exists(temp))File.Delete(temp);}
 }
 static void ValidateObs(string root){
  string exe=PathIn(root,"bin/64bit/obs64.exe");if(!File.Exists(exe))throw new IOException(T("Selecciona la carpeta que contiene bin/64bit/obs64.exe."));
  string version=FileVersionInfo.GetVersionInfo(exe).ProductVersion??"";
  if(!version.StartsWith("32.",StringComparison.Ordinal))throw new IOException(T("Se necesita OBS 32.x de Windows x64."));
  using(var r=new BinaryReader(File.OpenRead(exe))){if(r.ReadUInt16()!=0x5a4d)throw new IOException(T("OBS no es un ejecutable válido."));r.BaseStream.Position=0x3c;int pe=r.ReadInt32();r.BaseStream.Position=pe;if(r.ReadUInt32()!=0x4550||r.ReadUInt16()!=0x8664)throw new IOException(T("OBS debe ser x64."));}
 }
 static void RequireClosed(bool running){if(running)throw new IOException(T("Cierra OBS antes de instalar o desinstalar."));}
 static void RefuseRunning(){RequireClosed(Process.GetProcessesByName("obs64").Length!=0);}
 static bool IsPortable(string root){
  return new[]{"portable_mode","obs_portable_mode","portable_mode.txt","obs_portable_mode.txt"}.Any(name=>File.Exists(Path.Combine(root,name)));
 }
 static List<string> PluginDlls(string root){
  var folders=new List<string>{PathIn(root,"obs-plugins/64bit")};
  if(!FixtureMode&&!IsPortable(root))foreach(string basePath in new[]{Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.CommonApplicationData),"obs-studio","plugins"),Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),"obs-studio","plugins")}){
   if(!Directory.Exists(basePath))continue;folders.Add(basePath);foreach(string plugin in Directory.GetDirectories(basePath))folders.Add(Path.Combine(plugin,"bin","64bit"));
  }
  var files=new List<string>();foreach(string folder in folders)if(Directory.Exists(folder))files.AddRange(Directory.GetFiles(folder,"*.dll"));return files;
 }
 static bool OtherConsumers(string root){
  foreach(string path in PluginDlls(root)){
   if(Path.GetFileName(path).Equals("obs-dynamic-stream-title.dll",StringComparison.OrdinalIgnoreCase))continue;
   if(Path.GetFileName(path).Equals("obs-twitch-overlays.dll",StringComparison.OrdinalIgnoreCase))return true;
   if(new FileInfo(path).Length<100*1024*1024&&Encoding.ASCII.GetString(File.ReadAllBytes(path)).IndexOf("nexus-twitch-core.dll",StringComparison.OrdinalIgnoreCase)>=0)return true;
  }return false;
 }
 static void Install(string root){
  ValidateObs(root);byte[] plugin=Resource("plugin"),core=Resource("core");
  if(Hash(plugin)!=Payload.PluginHash||Hash(core)!=Payload.CoreHash||Hash(Resource("guide"))!=Payload.GuideHash)throw new IOException(T("La integridad del paquete no es correcta."));
  string pluginPath=PathIn(root,Plugin),corePath=PathIn(root,Core),manifestPath=PathIn(root,Manifest),sharedPath=PathIn(root,Shared);
  var prior=Read<InstallRecord>(manifestPath);if(prior!=null&&new Version(prior.Version)>new Version(Payload.Version))throw new IOException(T("Hay una versión posterior de Dynamic Stream Title."));
  var shared=Read<SharedRecord>(sharedPath);if(shared!=null&&(shared.Consumers==null||shared.Hash==null||shared.Abi<=0))throw new IOException(T("Registro del Core inválido."));
  string currentCore=FileHash(corePath);
  var overlayWrites=new Dictionary<string,byte[]>();
  foreach(string overlay in PluginDlls(root).Where(p=>Path.GetFileName(p).Equals("obs-twitch-overlays.dll",StringComparison.OrdinalIgnoreCase)))
   PlanOverlayUpgrade(overlay,overlayWrites);
  var packed=PackedContract("core-contract","core");
  if(packed.Version!=Payload.CoreVersion||packed.Abi!=Payload.CoreAbi)throw new IOException(T("Contrato de Twitch Core inválido."));
  var requirements=new List<CoreContract>{PackedContract("title-contract","consumer")};
  foreach(string file in PluginDlls(root)){
   string name=Path.GetFileName(file);if(name.Equals("obs-dynamic-stream-title.dll",StringComparison.OrdinalIgnoreCase))continue;
   CoreContract requirement=overlayWrites.ContainsKey(file)?PackedContract("overlay-contract","consumer"):DllContract(file);
   if(requirement!=null){requirements.Add(ValidateContract(requirement,"consumer"));continue;}
   if(name.Equals("obs-twitch-overlays.dll",StringComparison.OrdinalIgnoreCase)||new FileInfo(file).Length<100*1024*1024&&Encoding.ASCII.GetString(File.ReadAllBytes(file)).IndexOf("nexus-twitch-core.dll",StringComparison.OrdinalIgnoreCase)>=0)
    throw new IOException(String.Format(T("No se puede verificar la compatibilidad de {0}. Actualiza ese plugin; no se modificó Twitch Core."),name));
  }
  // Include registered consumers which are temporarily absent from the scanned folders.
  if(shared!=null){
   foreach(string id in shared.Consumers.Where(c=>c!=Consumer))if(!requirements.Any(r=>r.Consumer==id)){
    var requirement=shared.Requirements==null?null:shared.Requirements.FirstOrDefault(r=>r.Consumer==id);
    if(requirement==null)throw new IOException(String.Format(T("No se puede verificar la compatibilidad de {0}. Actualiza ese plugin; no se modificó Twitch Core."),id));
    requirements.Add(ValidateContract(requirement,"consumer"));
   }
  }
  CoreContract selected=packed;bool replace=currentCore==null;bool owned=currentCore==null||(shared!=null&&shared.OwnedByTitle);
  if(currentCore!=null){
   var installed=DllContract(corePath);
   if(currentCore==PreVersionCoreHash&&installed==null){
    if(shared!=null&&shared.Hash!=currentCore)throw new IOException(T("La integridad del paquete no es correcta."));
    replace=true; // Explicit migration of the pre-versioned runtime, never infer its version from Title.
   }else{
    ValidateContract(installed,"core");
    if(shared!=null&&shared.Hash!=currentCore)throw new IOException(T("La integridad del paquete no es correcta."));
    if(currentCore==Payload.CoreHash)selected=installed;
    else if(SemVer(installed.Version)>SemVer(packed.Version))selected=installed;
    else if(SemVer(installed.Version)==SemVer(packed.Version))throw new IOException(T("Misma versión de Twitch Core con contenido diferente. Se conserva sin reemplazar."));
    else{
     if(shared==null)throw new IOException(T("El Core existente pertenece a otro consumidor o no está identificado. Se conserva sin reemplazar."));
     replace=true;
    }
   }
  }
  foreach(var requirement in requirements)RequireCompatible(requirement,selected);
  if(shared==null)shared=new SharedRecord();shared.Abi=selected.Abi;shared.Version=selected.Version;shared.Hash=replace?Payload.CoreHash:currentCore;shared.OwnedByTitle=owned;
  shared.Requirements=requirements;shared.Consumers=requirements.Select(r=>r.Consumer).Distinct().ToList();
  var writes=new Dictionary<string,byte[]>(overlayWrites);writes[pluginPath]=plugin;if(replace)writes[corePath]=core;
  writes[sharedPath]=Encode(shared);writes[manifestPath]=Encode(new InstallRecord{Version=Payload.Version,PluginHash=Payload.PluginHash});
  writes[PathIn(root,"data/obs-plugins/obs-dynamic-stream-title/README.txt")]=Resource("readme");
  writes[PathIn(root,"data/obs-plugins/obs-dynamic-stream-title/LICENSE.txt")]=Resource("license");
  writes[PathIn(root,"data/obs-plugins/obs-dynamic-stream-title/User-Guide-5-Languages.pdf")]=Resource("guide");
  var old=new Dictionary<string,byte[]>();foreach(string path in writes.Keys)old[path]=File.Exists(path)?File.ReadAllBytes(path):null;
  var completed=new List<string>();try{foreach(var pair in writes){Atomic(pair.Key,pair.Value,true);completed.Add(pair.Key);}}
  catch{foreach(string path in completed.AsEnumerable().Reverse()){if(old[path]==null)File.Delete(path);else Atomic(path,old[path],false);}throw;}
 }
 static void Uninstall(string root){
  ValidateObs(root);string manifestPath=PathIn(root,Manifest);var record=Read<InstallRecord>(manifestPath);if(record==null)return;
  string pluginPath=PathIn(root,Plugin);if(File.Exists(pluginPath)&&FileHash(pluginPath)!=record.PluginHash)throw new IOException(T("El plugin fue sustituido por otra versión. Se conserva sin borrar."));
  var shared=Read<SharedRecord>(PathIn(root,Shared));
  if(shared!=null&&(shared.Consumers==null||shared.Hash==null))throw new IOException(T("Registro del Core inválido; no se modifica."));
  if(shared!=null){shared.Consumers.RemoveAll(c=>c==Consumer);if(shared.Requirements!=null)shared.Requirements.RemoveAll(c=>c.Consumer==Consumer);if(shared.Consumers.Count==0&&shared.OwnedByTitle&&!OtherConsumers(root)&&FileHash(PathIn(root,Core))==shared.Hash){File.Delete(PathIn(root,Core));File.Delete(PathIn(root,Shared));}else Atomic(PathIn(root,Shared),Encode(shared),true);}
  if(File.Exists(pluginPath))File.Delete(pluginPath);
  // Keep user state, credentials, backups, other plugins and unknown data files.
  foreach(string relative in new[]{"data/obs-plugins/obs-dynamic-stream-title/README.txt","data/obs-plugins/obs-dynamic-stream-title/LICENSE.txt"}){
   string path=PathIn(root,relative);string resource=relative.EndsWith("README.txt")?"readme":"license";
   if(File.Exists(path)&&FileHash(path)==Hash(Resource(resource)))File.Delete(path);
  }string guide=PathIn(root,"data/obs-plugins/obs-dynamic-stream-title/User-Guide-5-Languages.pdf");if(File.Exists(guide)&&FileHash(guide)==Hash(Resource("guide")))File.Delete(guide);File.Delete(manifestPath);
 }
 static bool Admin(){return new WindowsPrincipal(WindowsIdentity.GetCurrent()).IsInRole(WindowsBuiltInRole.Administrator);}
 static void Execute(string action,string root){
  RefuseRunning();try{if(action=="/uninstall")Uninstall(root);else Install(root);}
  catch(UnauthorizedAccessException){if(Admin())throw;var start=new ProcessStartInfo(Application.ExecutablePath,action+" \""+root+"\""){UseShellExecute=true,Verb="runas"};using(var p=Process.Start(start)){p.WaitForExit();if(p.ExitCode!=0)throw new IOException(T("No se completó la operación con permisos de administrador."));}}
 }
 static void SelfTest(string fixture,string obs,string log){
  FixtureMode=true;
  T("Idioma");foreach(var row in Translations)foreach(string code in new[]{"es","en","it","fr","de"})if(!row.Value.ContainsKey(code)||String.IsNullOrWhiteSpace(row.Value[code]))throw new Exception("Missing translation: "+row.Key+" "+code);
  if(Hash(Resource("guide"))!=Payload.GuideHash||Hash(Resource("branding"))!=Payload.BrandingHash)throw new Exception("Guide or branding mismatch.");
  if(Directory.Exists(fixture))throw new IOException("La carpeta de pruebas ya existe.");Directory.CreateDirectory(fixture);
  string root=Path.Combine(fixture,"OBS prueba ñ");Directory.CreateDirectory(Path.Combine(root,"bin","64bit"));File.Copy(obs,Path.Combine(root,"bin","64bit","obs64.exe"));
  foreach(string marker in new[]{"portable_mode","obs_portable_mode","portable_mode.txt","obs_portable_mode.txt"}){string path=Path.Combine(root,marker);File.WriteAllText(path,"");if(!IsPortable(root))throw new Exception("Portable marker ignored.");File.Delete(path);}
  File.WriteAllText(Path.Combine(root,"portable_mode.txt"),"");FixtureMode=false;
  if(PluginDlls(root).Count!=0)throw new Exception("Portable scans global plugins.");
  string localOverlay=PathIn(root,"obs-plugins/64bit/obs-twitch-overlays.dll");Atomic(localOverlay,Encoding.UTF8.GetBytes("incompatible-overlay"),false);
  bool overlayRefused=false;try{Install(root);}catch{overlayRefused=true;}if(!overlayRefused||!PluginDlls(root).Contains(localOverlay))throw new Exception("Portable local Overlay not protected.");File.Delete(localOverlay);FixtureMode=true;
  Install(root);if(FileHash(PathIn(root,Plugin))!=Payload.PluginHash||FileHash(PathIn(root,Core))!=Payload.CoreHash||FileHash(PathIn(root,"data/obs-plugins/obs-dynamic-stream-title/User-Guide-5-Languages.pdf"))!=Payload.GuideHash)throw new Exception("Extracción incorrecta.");
  Install(root);if(Directory.GetFiles(Path.GetDirectoryName(PathIn(root,Plugin)),"*.dst-backup-*").Length==0)throw new Exception("Falta backup.");
  string unrelated=PathIn(root,"obs-plugins/64bit/unrelated.dll");File.WriteAllText(unrelated,"keep");
  string config=PathIn(root,"config/obs-studio/plugin_config/obs-dynamic-stream-title/state.json");Atomic(config,Encoding.UTF8.GetBytes("user-data"),false);
  Uninstall(root);Uninstall(root);if(File.Exists(PathIn(root,Plugin))||File.Exists(PathIn(root,Core))||File.ReadAllText(unrelated)!="keep"||File.ReadAllText(config)!="user-data")throw new Exception("Desinstalación incorrecta.");
  Install(root);var shared=Read<SharedRecord>(PathIn(root,Shared));shared.Consumers.Add("other-plugin");shared.Requirements.Add(new CoreContract{Schema=1,Kind="consumer",Consumer="other-plugin",Abi=6,MinVersion="1.0.0",MaxVersionExclusive="2.0.0"});Atomic(PathIn(root,Shared),Encode(shared),false);Uninstall(root);if(!File.Exists(PathIn(root,Core)))throw new Exception("Core compartido eliminado.");
  Install(root);Atomic(PathIn(root,Shared),Encoding.UTF8.GetBytes("invalid-json"),false);string before=FileHash(PathIn(root,Plugin));bool refused=false;try{Install(root);}catch{refused=true;}if(!refused||FileHash(PathIn(root,Plugin))!=before)throw new Exception("Manifiesto inválido aceptado.");
  Atomic(PathIn(root,Shared),Encode(shared),false);File.WriteAllText(PathIn(root,Core),"unknown-core");refused=false;try{Install(root);}catch{refused=true;}if(!refused||File.ReadAllText(PathIn(root,Core))!="unknown-core")throw new Exception("Core desconocido reemplazado.");
  Atomic(PathIn(root,Core),Resource("core"),false);Atomic(PathIn(root,Manifest),Encode(new InstallRecord{Version="99.0.0",PluginHash=Payload.PluginHash}),false);refused=false;try{Install(root);}catch{refused=true;}if(!refused)throw new Exception("Downgrade aceptado.");
  refused=false;try{RequireClosed(true);}catch{refused=true;}if(!refused)throw new Exception("OBS abierto aceptado.");
  refused=false;try{ValidateObs(Path.Combine(fixture,"invalid"));}catch{refused=true;}if(!refused)throw new Exception("Carpeta inválida aceptada.");
  File.WriteAllText(log,"PASS: portable marker variants, global plugin isolation, local Overlay protection, SHA256, OBS32 x64, Unicode, fresh install, reinstall, backups, repeated uninstall, unrelated data preserved, shared Core retained, invalid manifest refused, unknown Core preserved, downgrade refused, running OBS guard, invalid OBS path refused. No real OBS installation or registry changed.\n");
 }
 [STAThread]static int Main(string[] args){
  try{
   if(args.Length==4&&args[0]=="/selftest"){SelfTest(args[1],args[2],args[3]);return 0;}
   if(args.Length==2&&(args[0]=="/install"||args[0]=="/uninstall")){Execute(args[0],args[1]);return 0;}
#if DST_UI_RENDER
   bool render=args.Length==3&&args[0]=="/render";if(render)Language=args[1];
#endif
   bool preview=args.Length==2&&args[0]=="/preview";if(preview)Language=args[1];
   Application.EnableVisualStyles();var form=new Form{Icon=Icon.ExtractAssociatedIcon(Application.ExecutablePath),Text="Dynamic Stream Title for Twitch — "+Payload.Version,ClientSize=new Size(920,500),AutoScaleMode=AutoScaleMode.Dpi,StartPosition=FormStartPosition.CenterScreen,BackColor=Color.FromArgb(20,22,34),ForeColor=Color.White,Font=new Font("Segoe UI",9),FormBorderStyle=FormBorderStyle.FixedDialog,MaximizeBox=false};
   var artwork=new PictureBox{Left=0,Top=0,Width=333,Height=500,SizeMode=PictureBoxSizeMode.Zoom};using(var stream=new MemoryStream(Resource("branding")))using(var image=Image.FromStream(stream))artwork.Image=new Bitmap(image);form.Controls.Add(artwork);
   form.Controls.Add(new Label{Text="Dynamic Stream Title",Left=360,Top=65,Width=530,Height=42,Font=new Font("Segoe UI",22,FontStyle.Bold)});
   form.Controls.Add(new Label{Text="for Twitch",Left=362,Top=110,Width=530,Height=28,Font=new Font("Segoe UI",13),ForeColor=Color.FromArgb(179,157,234)});
   var languageLabel=new Label{Left=360,Top=20,Width=95,Height=22};form.Controls.Add(languageLabel);
   var languageBox=new ComboBox{Left=460,Top=16,Width=215,DropDownStyle=ComboBoxStyle.DropDownList};string[] codes={"es","en","it","fr","de"};languageBox.Items.AddRange(new object[]{"Español","English","Italiano","Français","Deutsch"});form.Controls.Add(languageBox);
   var description=new Label{Left=360,Top=158,Width=530,Height=62};form.Controls.Add(description);
   var folder=new TextBox{Left=360,Top=244,Width=420,Text=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles),"obs-studio")};form.Controls.Add(folder);
   var browse=new Button{Left=790,Top=241,Width=100,Height=30,FlatStyle=FlatStyle.Flat};browse.Click+=(s,e)=>{using(var pick=new FolderBrowserDialog{SelectedPath=folder.Text,Description=T("Selecciona la carpeta raíz de OBS 32.x")})if(pick.ShowDialog()==DialogResult.OK)folder.Text=pick.SelectedPath;};form.Controls.Add(browse);
   var info=new Label{Left=360,Top=291,Width=530,Height=66};form.Controls.Add(info);
   var guideButton=new Button{Left=360,Top=368,Width=530,Height=35,FlatStyle=FlatStyle.Flat};guideButton.Click+=(s,e)=>{try{string file=Path.Combine(Path.GetTempPath(),"Dynamic-Stream-Title-Guide-"+Payload.Version+".pdf");Atomic(file,Resource("guide"),false);Process.Start(new ProcessStartInfo(file){UseShellExecute=true});}catch(Exception error){MessageBox.Show(form,error.Message,"Dynamic Stream Title");}};form.Controls.Add(guideButton);
   var actions=new List<Button>();foreach(string action in new[]{"/install","/uninstall"}){string selected=action;var button=new Button{Left=action=="/install"?360:635,Top=424,Width=255,Height=45,FlatStyle=FlatStyle.Flat,BackColor=action=="/install"?Color.FromArgb(103,65,187):Color.FromArgb(37,42,62)};button.Click+=(s,e)=>{try{Execute(selected,Path.GetFullPath(folder.Text));info.Text=T("Operación completada. Configuración del usuario conservada.");}catch(Exception error){MessageBox.Show(form,error.Message,"Dynamic Stream Title",MessageBoxButtons.OK,MessageBoxIcon.Warning);}};actions.Add(button);form.Controls.Add(button);}
   Action translate=()=>{form.Text="Dynamic Stream Title for Twitch — "+Payload.Version+(preview?" ["+Language+"]":"");languageLabel.Text=T("Idioma");description.Text=T("OBS 32.x · Windows x64 · versión oficial")+"\nTwitch Core "+Payload.CoreVersion+" · ABI "+Payload.CoreAbi;browse.Text=T("Carpeta…");info.Text=T("Cierra OBS antes de continuar. Tu configuración se conserva.")+"\n"+T("Actualiza Overlay 0.14.5 reconocido con backup; conserva presets y ajustes.");guideButton.Text=T("Guía de uso PDF");actions[0].Text=T("Instalar / actualizar");actions[1].Text=T("Desinstalar");};
   languageBox.SelectedIndexChanged+=(s,e)=>{Language=codes[languageBox.SelectedIndex];translate();};int languageIndex=Array.IndexOf(codes,Language);languageBox.SelectedIndex=languageIndex<0?1:languageIndex;translate();
#if DST_UI_RENDER
   if(render){
    Action<Control> create=null;create=control=>{var handle=control.Handle;foreach(Control child in control.Controls)create(child);};create(form);languageBox.SelectedIndex=languageIndex<0?1:languageIndex;form.PerformLayout();Application.DoEvents();
    using(var bitmap=new Bitmap(form.Width,form.Height)){
     form.DrawToBitmap(bitmap,new Rectangle(Point.Empty,form.Size));
     // WinForms WM_PRINT omits the selection text of an invisible ComboBox.
     using(var graphics=Graphics.FromImage(bitmap)){var origin=form.PointToScreen(Point.Empty);origin.Offset(-form.Left,-form.Top);var area=languageBox.Bounds;area.Offset(origin);area.Inflate(-4,-2);graphics.FillRectangle(Brushes.White,area);TextRenderer.DrawText(graphics,languageBox.SelectedItem as string??Language,languageBox.Font,area,Color.Black,TextFormatFlags.VerticalCenter|TextFormatFlags.Left);}
     bitmap.Save(args[2],System.Drawing.Imaging.ImageFormat.Png);
    }form.Dispose();return 0;
   }
#endif
   Application.Run(form);return 0;
  }catch(Exception error){if(args.Length==4&&args[0]=="/selftest")File.WriteAllText(args[3],"FAIL: "+error);else MessageBox.Show(error.Message,"Dynamic Stream Title",MessageBoxButtons.OK,MessageBoxIcon.Error);return 1;}
 }
}
