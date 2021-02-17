(*  Initial Developer's Public License.
  The contents of this file are subject to the  Initial Developer's Public
  License Version 1.0 (the "License"). You may not use this file except
  in compliance with the License. You may obtain a copy of the License at
    http://www.ibphoenix.com?a=ibphoenix&page=ibp_idpl
  Software distributed under the License is distributed on an "AS IS" basis,
  WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
  for the specific language governing rights and limitations under the
  License.

  The Original Code is copyright 2001-2003 Paul Reeves for IBPhoenix.

  The Initial Developer of the Original Code is Paul Reeves for IBPhoenix.

  All Rights Reserved.

  This file contains a whole bunch of functions that
  check the registry and see what versions of firebird or interbase
  are already installed.

  This stuff _is_ firebird/interbase specific and some of it is entirely dependant
  upon parts of the InnoSetup install script.

  Here is a partial list of functions available in this script:

    function ClassicInstallChosen: Boolean;

    procedure SetupSharedFilesArray;
    procedure GetSharedLibCountBeforeCopy;
    procedure CheckSharedLibCountAtEnd;

    function GetFirebirdDir: string;
    function GetInterBaseDir: string;
    function TestForWorkingInstalls: boolean;

    function FirebirdOneRunning: boolean;

    function InstallCPLApplet: boolean;
    function ShowInstallCPLAppletTask: boolean;
    function CopyGds32: boolean;
    function ShowCopyGds32Task: boolean;
    function CopyFbClientLib: boolean;
    function ShowCopyFbClientLibTask: boolean;

    function HasWI30: boolean;
    function HasNotWI30: boolean;
		
		function SummarizeInstalledProducts: String;
		function AnalysisAssessment: boolean;

 *)


//Registry keys for Firebird and InterBase
Const
  //All InterBase and Firebird 1.0.n except IB5.n
  IBRegKey            = 'SOFTWARE\Borland\InterBase\CurrentVersion';
  //IB5.n
  IB5RegKey           = 'SOFTWARE\InterBase Corp\InterBase\CurrentVersion';
  //Fb15 RC
  FB15RCKey           = 'SOFTWARE\FirebirdSQL\Firebird\CurrentVersion';
  FB15RCKeyRoot       = 'SOFTWARE\FirebirdSQL';

  //All IB, Fb 1.0  and Fb 1.5RC's use RootDirectory entry
  LegacyRegPathEntry  = 'RootDirectory';

  //Firebird 1.5 and beyond
  FB2RegKey       = 'SOFTWARE\Firebird Project\Firebird Server\Instances';

  FBRegPathEntry  = 'DefaultInstance';    //Stores path to root

  IB4MessageFile  = 'interbas.msg';
  IBMessageFile   = 'interbase.msg';      //IB5, IB6, IB7 and Fb 1.0
  FBMessageFile   = 'firebird.msg';       //Fb2 codebase

  IBDesc = 'InterBase %s ';
  FBDesc = 'Firebird %s ';



Const
  //Install Types
  NotInstalled          = 0;
  ClientInstall         = 1;
  AdminInstall          = 2;
  SuperServerInstall    = 4;
  ClassicServerInstall  = 8;
  BrokenInstall         = 32;   //version or component mismatch found, so mark broken

  //Possible product installs
  IB4Install        = 0;
  IB5Install        = 1;
  IB6Install        = 2;
  IB65Install       = 3;
  IB70Install       = 4;
  FB1Install        = 5;
  FB15RCInstall     = 6;
  FB15Install       = 7;
  FB20Install       = 8;
  IB80Install       = 9;
  IB81Install       = 10;
  FB21Install       = 11;
  FB21_x64_Install  = 12;
  FB25Install       = 13;
  FB25_x64_Install  = 14;
  FB30Install       = 15;
  FB30_x64_Install  = 16;

  MaxProdInstalled = FB30_x64_Install;

//ProductsInstalled
  IB4       = $00001;
  IB5       = $00002;
  IB6       = $00004;
  IB65      = $00008;
  IB70      = $00010;
  FB1       = $00020;
  FB15RC    = $00040;
  FB15      = $00080;
  FB20      = $00100;
  IB80      = $00200;
  IB81      = $00400;
  FB21      = $00800;
  FB25      = $01000;
  FB30      = $02000;
  FB21_x64  = $04000;
  FB25_x64  = $08000;
  FB30_x64  = $10000;

  //  Likely gds32 version strings for installed versions of Firebird or InterBase are:
  //  [6,0,n,n]     InterBase 6.0
  //  [6,2,0,nnn]   Firebird 1.0.0
  //  [6,2,2,nnn]   Firebird 1.0.2
  //  [6,2,3,nnn]   Firebird 1.0.3
  //  [6,5,n,n]     InterBase 6.5
  //  [6,3,0,nnnn]  Any version of Firebird since 1.5.0. Not reliable for Fb detection
  //  [7,n,n,n]     InterBase 7


Const
  Install   = 1;
  Configure = 2;

Var
  ProductsInstalled: Integer;
  ProductsInstalledCount: Integer;
  InstallAndConfigure: Integer;

Type
  TProduct = record
    ProductID:      Integer;
    Description:    String;
    RegKey:         String;
    RegEntry:       String;
    RegVersion:     String;
    MessageFile:    String;
    Path:           String;
    ClientVersion:  String;
    GBAKVersion:    String;
    ServerVersion:  String;
    InstallType:    Integer;
    ActualVersion:  String;
    FirebirdVersion:String;
    RootKey:        Integer;
    BinPath:        String;
  end;


Var
  ProductsInstalledArray: Array of TProduct;

procedure InitExistingInstallRecords;
var
  product: Integer;
begin

  SetArrayLength(ProductsInstalledArray,MaxProdInstalled + 1);
  for product := 0 to MaxProdInstalled do begin

     ProductsInstalledArray[product].ProductID := product;

    case product of

      IB4Install: begin
        ProductsInstalledArray[product].Description := IBDesc;
        ProductsInstalledArray[product].RegKey := IBRegKey;
        ProductsInstalledArray[product].RegEntry := LegacyRegPathEntry;
        ProductsInstalledArray[product].MessageFile := IB4MessageFile;
        ProductsInstalledArray[product].RootKey := HKLM32;
        ProductsInstalledArray[product].Path := GetRegistryEntry(ProductsInstalledArray[product].RootKey,
          ProductsInstalledArray[product].RegKey, ProductsInstalledArray[product].RegEntry);
        ProductsInstalledArray[product].BinPath := ProductsInstalledArray[product].Path + 'bin\' ;
      end;

      IB5Install: begin
        ProductsInstalledArray[product].Description := IBDesc;
        ProductsInstalledArray[product].RegKey := IB5RegKey;
        ProductsInstalledArray[product].RegEntry := LegacyRegPathEntry;
        ProductsInstalledArray[product].MessageFile := IBMessageFile;
        ProductsInstalledArray[product].RootKey := HKLM32;
        ProductsInstalledArray[product].Path := GetRegistryEntry(ProductsInstalledArray[product].RootKey,
          ProductsInstalledArray[product].RegKey, ProductsInstalledArray[product].RegEntry);
        ProductsInstalledArray[product].BinPath := ProductsInstalledArray[product].Path + 'bin\' ;
      end;

      IB6Install: begin
        ProductsInstalledArray[product].Description := IBDesc;
        ProductsInstalledArray[product].RegKey := IBRegKey;
        ProductsInstalledArray[product].RegEntry := LegacyRegPathEntry;
        ProductsInstalledArray[product].MessageFile := IBMessageFile;
        ProductsInstalledArray[product].RootKey := HKLM32;
        ProductsInstalledArray[product].Path := GetRegistryEntry(ProductsInstalledArray[product].RootKey,
          ProductsInstalledArray[product].RegKey, ProductsInstalledArray[product].RegEntry);
        ProductsInstalledArray[product].BinPath := ProductsInstalledArray[product].Path + 'bin\' ;
      end;

      IB65Install: begin
        ProductsInstalledArray[product].Description := IBDesc;
        ProductsInstalledArray[product].RegKey := IBRegKey;
        ProductsInstalledArray[product].RegEntry := LegacyRegPathEntry;
        ProductsInstalledArray[product].MessageFile := IBMessageFile;
        ProductsInstalledArray[product].RootKey := HKLM32;
        ProductsInstalledArray[product].Path := GetRegistryEntry(ProductsInstalledArray[product].RootKey,
          ProductsInstalledArray[product].RegKey, ProductsInstalledArray[product].RegEntry);
        ProductsInstalledArray[product].BinPath := ProductsInstalledArray[product].Path + 'bin\' ;
      end;

      IB70Install: begin
        ProductsInstalledArray[product].Description := IBDesc;
        ProductsInstalledArray[product].RegKey := IBRegKey;
        ProductsInstalledArray[product].RegEntry := LegacyRegPathEntry;
        ProductsInstalledArray[product].MessageFile := IBMessageFile;
        ProductsInstalledArray[product].RootKey := HKLM32;
        ProductsInstalledArray[product].Path := GetRegistryEntry(ProductsInstalledArray[product].RootKey,
          ProductsInstalledArray[product].RegKey, ProductsInstalledArray[product].RegEntry);
        ProductsInstalledArray[product].BinPath := ProductsInstalledArray[product].Path + 'bin\' ;
      end;

      IB80Install: begin
        ProductsInstalledArray[product].Description := IBDesc;
        ProductsInstalledArray[product].RegKey := IBRegKey;
        ProductsInstalledArray[product].RegEntry := LegacyRegPathEntry;
        ProductsInstalledArray[product].MessageFile := IBMessageFile;
        ProductsInstalledArray[product].RootKey := HKLM32;
        ProductsInstalledArray[product].Path := GetRegistryEntry(ProductsInstalledArray[product].RootKey,
          ProductsInstalledArray[product].RegKey, ProductsInstalledArray[product].RegEntry);
        ProductsInstalledArray[product].BinPath := ProductsInstalledArray[product].Path + 'bin\' ;
      end;

      IB81Install: begin
        ProductsInstalledArray[product].Description := IBDesc;
        ProductsInstalledArray[product].RegKey := IBRegKey;
        ProductsInstalledArray[product].RegEntry := LegacyRegPathEntry;
        ProductsInstalledArray[product].MessageFile := IBMessageFile;
        ProductsInstalledArray[product].RootKey := HKLM32;
        ProductsInstalledArray[product].Path := GetRegistryEntry(ProductsInstalledArray[product].RootKey,
          ProductsInstalledArray[product].RegKey, ProductsInstalledArray[product].RegEntry);
        ProductsInstalledArray[product].BinPath := ProductsInstalledArray[product].Path + 'bin\' ;
      end;

      FB1Install: begin
        ProductsInstalledArray[product].Description := FBDesc;
        ProductsInstalledArray[product].RegKey := IBRegKey;
        ProductsInstalledArray[product].RegEntry := LegacyRegPathEntry;
        ProductsInstalledArray[product].MessageFile := IBMessageFile;
        ProductsInstalledArray[product].RootKey := HKLM32;
        ProductsInstalledArray[product].Path := GetRegistryEntry(ProductsInstalledArray[product].RootKey,
          ProductsInstalledArray[product].RegKey, ProductsInstalledArray[product].RegEntry);
        ProductsInstalledArray[product].BinPath := ProductsInstalledArray[product].Path + 'bin\' ;
      end;

      FB15RCInstall: begin
        ProductsInstalledArray[product].Description := FBDesc;
        ProductsInstalledArray[product].RegKey := FB15RCKey;
        ProductsInstalledArray[product].RegEntry := LegacyRegPathEntry;
        ProductsInstalledArray[product].MessageFile := FBMessageFile;
        ProductsInstalledArray[product].RootKey := HKLM32;
        ProductsInstalledArray[product].Path := GetRegistryEntry(ProductsInstalledArray[product].RootKey,
          ProductsInstalledArray[product].RegKey, ProductsInstalledArray[product].RegEntry);
        ProductsInstalledArray[product].BinPath := ProductsInstalledArray[product].Path + 'bin\' ;
      end;

      FB15Install: begin
        ProductsInstalledArray[product].Description := FBDesc;
        ProductsInstalledArray[product].RegKey := FB2RegKey;
        ProductsInstalledArray[product].RegEntry := FBRegPathEntry;
        ProductsInstalledArray[product].MessageFile := FBMessageFile;
        ProductsInstalledArray[product].RootKey := HKLM32;
        ProductsInstalledArray[product].Path := GetRegistryEntry(ProductsInstalledArray[product].RootKey,
          ProductsInstalledArray[product].RegKey, ProductsInstalledArray[product].RegEntry);
        ProductsInstalledArray[product].BinPath := ProductsInstalledArray[product].Path + 'bin\' ;
      end;

      FB20Install: begin
        ProductsInstalledArray[product].Description := FBDesc;
        ProductsInstalledArray[product].RegKey := FB2RegKey;
        ProductsInstalledArray[product].RegEntry := FBRegPathEntry;
        ProductsInstalledArray[product].MessageFile := FBMessageFile;
        ProductsInstalledArray[product].RootKey := HKLM32;
        ProductsInstalledArray[product].Path := GetRegistryEntry(ProductsInstalledArray[product].RootKey,
          ProductsInstalledArray[product].RegKey, ProductsInstalledArray[product].RegEntry);
        ProductsInstalledArray[product].BinPath := ProductsInstalledArray[product].Path + 'bin\' ;
      end;

      FB21Install: begin
        ProductsInstalledArray[product].Description := FBDesc + ' (Win32) ';
        ProductsInstalledArray[product].RegKey := FB2RegKey;
        ProductsInstalledArray[product].RegEntry := FBRegPathEntry;
        ProductsInstalledArray[product].MessageFile := FBMessageFile;
        ProductsInstalledArray[product].RootKey := HKLM32;
        ProductsInstalledArray[product].Path := GetRegistryEntry(ProductsInstalledArray[product].RootKey,
          ProductsInstalledArray[product].RegKey, ProductsInstalledArray[product].RegEntry);
        ProductsInstalledArray[product].BinPath := ProductsInstalledArray[product].Path + 'bin\' ;
      end;

      FB21_x64_Install: begin
        ProductsInstalledArray[product].Description := FBDesc + ' (x64) ';
        ProductsInstalledArray[product].RegKey := FB2RegKey;
        ProductsInstalledArray[product].RegEntry := FBRegPathEntry;
        ProductsInstalledArray[product].MessageFile := FBMessageFile;
        ProductsInstalledArray[product].RootKey := HKLM64;
        ProductsInstalledArray[product].Path := GetRegistryEntry(ProductsInstalledArray[product].RootKey,
          ProductsInstalledArray[product].RegKey, ProductsInstalledArray[product].RegEntry);
        ProductsInstalledArray[product].BinPath := ProductsInstalledArray[product].Path + 'bin\' ;
      end;

      FB25Install: begin
        ProductsInstalledArray[product].Description := FBDesc + ' (Win32) ';
        ProductsInstalledArray[product].RegKey := FB2RegKey;
        ProductsInstalledArray[product].RegEntry := FBRegPathEntry;
        ProductsInstalledArray[product].MessageFile := FBMessageFile;
        ProductsInstalledArray[product].RootKey := HKLM32;
        ProductsInstalledArray[product].Path := GetRegistryEntry(ProductsInstalledArray[product].RootKey,
          ProductsInstalledArray[product].RegKey, ProductsInstalledArray[product].RegEntry);
        ProductsInstalledArray[product].BinPath := ProductsInstalledArray[product].Path + 'bin\' ;
      end;

      FB25_x64_Install: begin
        ProductsInstalledArray[product].Description := FBDesc + ' (x64) ';
        ProductsInstalledArray[product].RegKey := FB2RegKey;
        ProductsInstalledArray[product].RegEntry := FBRegPathEntry;
        ProductsInstalledArray[product].MessageFile := FBMessageFile;
        ProductsInstalledArray[product].RootKey := HKLM64;
        ProductsInstalledArray[product].Path := GetRegistryEntry(ProductsInstalledArray[product].RootKey,
          ProductsInstalledArray[product].RegKey, ProductsInstalledArray[product].RegEntry);
        ProductsInstalledArray[product].BinPath := ProductsInstalledArray[product].Path + 'bin\' ;
      end;

      FB30Install: begin
        ProductsInstalledArray[product].Description := FBDesc + ' (Win32) ';
        ProductsInstalledArray[product].RegKey := FB2RegKey;
        ProductsInstalledArray[product].RegEntry := FBRegPathEntry;
        ProductsInstalledArray[product].MessageFile := FBMessageFile;
        ProductsInstalledArray[product].RootKey := HKLM32;
        ProductsInstalledArray[product].Path := GetRegistryEntry(ProductsInstalledArray[product].RootKey,
          ProductsInstalledArray[product].RegKey, ProductsInstalledArray[product].RegEntry);
        ProductsInstalledArray[product].BinPath := ProductsInstalledArray[product].Path;
      end;

      FB30_x64_Install: begin
        ProductsInstalledArray[product].Description := FBDesc + ' (x64) ';
        ProductsInstalledArray[product].RegKey := FB2RegKey;
        ProductsInstalledArray[product].RegEntry := FBRegPathEntry;
        ProductsInstalledArray[product].MessageFile := FBMessageFile;
        ProductsInstalledArray[product].RootKey := HKLM64;
        ProductsInstalledArray[product].Path := GetRegistryEntry(ProductsInstalledArray[product].RootKey,
          ProductsInstalledArray[product].RegKey, ProductsInstalledArray[product].RegEntry);
        ProductsInstalledArray[product].BinPath := ProductsInstalledArray[product].Path;
      end;

    end; //case

    ProductsInstalledArray[product].RegVersion := GetRegistryEntry(
      ProductsInstalledArray[product].RootKey, ProductsInstalledArray[product].RegKey, 'Version');

  end;  //for
end; //function


Procedure AnalyzeEnvironment;
var
  product: Integer;
  gds32VersionString: String;
  VerInt: Array of Integer;
  BoolOne, BoolTwo, BoolEval: Boolean;
  EvalOne, EvalTwo: Integer;

  dbg_ProductPath, dbg_BinPath, dbg_ClientVersion, dbg_GBAKVersion, dbg_Server: String;
  dbg_InstallType : Integer;
  eval_bool: boolean;


begin

  ProductsInstalled := 0;
  ProductsInstalledCount := 0;

  //Test for gds32 version in <sys>
  if FileExists(GetSysPath+'\gds32.dll') then begin
    gds32VersionString := GetInstalledVersion(GetSysPath+'\gds32.dll',VerInt);
  end;

  for product := 0 to MaxProdInstalled do begin

    // Check if working client already installed.
    ///////////////////////
      dbg_ProductPath := ProductsInstalledArray[product].Path;
      dbg_BinPath := ProductsInstalledArray[product].BinPath;
      dbg_ClientVersion := ProductsInstalledArray[product].ClientVersion
      dbg_GBAKVersion := ProductsInstalledArray[product].GBAKVersion;
      dbg_Server := ProductsInstalledArray[product].ServerVersion;
      dbg_InstallType := ProductsInstalledArray[product].InstallType;

    if FileExists(ProductsInstalledArray[product].BinPath + '\fbclient.dll') then
      ProductsInstalledArray[product].ClientVersion := GetInstalledVersion(
        ProductsInstalledArray[product].BinPath + '\fbclient.dll',VerInt)
      else
        ProductsInstalledArray[product].ClientVersion := gds32VersionString;

    If (ProductsInstalledArray[product].Path<>'') AND (ProductsInstalledArray[product].ClientVersion <> '') AND
      (FileExists(ProductsInstalledArray[product].Path+'\'+ProductsInstalledArray[product].MessageFile)) then
        ProductsInstalledArray[product].InstallType := ProductsInstalledArray[product].InstallType + ClientInstall
    else
      //The minimum requirements for a working client don't exist, so ignore this product.
      Continue;

    // Client version found, so see what else is there
    ///////////
    if ((ProductsInstalledArray[product].InstallType AND ClientInstall) = ClientInstall) then begin

      GetVersionNumbersString( ProductsInstalledArray[product].BinPath+'\gbak.exe',
          ProductsInstalledArray[product].GBAKVersion);
      If ProductsInstalledArray[product].GBAKVersion <> '' then begin
        ProductsInstalledArray[product].ActualVersion := ProductsInstalledArray[product].GBAKVersion;
        ProductsInstalledArray[product].InstallType := ProductsInstalledArray[product].InstallType + AdminInstall;
      end;

      if FileExists(ProductsInstalledArray[product].BinPath+'\fb_inet_server.exe') then begin
        GetVersionNumbersString( ProductsInstalledArray[product].BinPath+'\fb_inet_server.exe',
          ProductsInstalledArray[product].ServerVersion);
        If ProductsInstalledArray[product].ServerVersion <> '' then begin
          ProductsInstalledArray[product].ActualVersion := ProductsInstalledArray[product].ServerVersion;
          ProductsInstalledArray[product].InstallType := ProductsInstalledArray[product].InstallType + ServerInstall;
        end;
      end;

      if FileExists(ProductsInstalledArray[product].BinPath+'\fbserver.exe') then begin
        GetVersionNumbersString( ProductsInstalledArray[product].BinPath+'\fbserver.exe',
          ProductsInstalledArray[product].ServerVersion);
        If ProductsInstalledArray[product].ServerVersion <> '' then begin
          ProductsInstalledArray[product].ActualVersion := ProductsInstalledArray[product].ServerVersion;
          ProductsInstalledArray[product].InstallType := ProductsInstalledArray[product].InstallType + ServerInstall;
        end;
      end;

      if FileExists(ProductsInstalledArray[product].BinPath+'\ibserver.exe') then begin
        GetVersionNumbersString( ProductsInstalledArray[product].BinPath+'\ibserver.exe',
          ProductsInstalledArray[product].ServerVersion);
        If ProductsInstalledArray[product].ServerVersion <> '' then begin
          ProductsInstalledArray[product].ActualVersion := ProductsInstalledArray[product].ServerVersion;
          ProductsInstalledArray[product].InstallType := ProductsInstalledArray[product].InstallType + ServerInstall;
        end;
      end;

      if FileExists(ProductsInstalledArray[product].BinPath+'\firebird.exe') then begin
        GetVersionNumbersString( ProductsInstalledArray[product].BinPath+'\firebird.exe',
          ProductsInstalledArray[product].ServerVersion);
        If ProductsInstalledArray[product].ServerVersion <> '' then begin
          ProductsInstalledArray[product].ActualVersion := ProductsInstalledArray[product].ServerVersion;
          ProductsInstalledArray[product].InstallType := ProductsInstalledArray[product].InstallType + ServerInstall;
        end;
      end;

      if (ProductsInstalledArray[product].InstallType <> NotInstalled) then begin
        //Check that we haven't already flagged the install as broken.
        // AND ((ProductsInstalledArray[product].InstallType AND BrokenInstall)<>BrokenInstall))
          //Now test that the version strings match!
          if (CompareStr(ProductsInstalledArray[product].ClientVersion, ProductsInstalledArray[product].GBAKVersion)<> 0) then
            ProductsInstalledArray[product].InstallType := ProductsInstalledArray[product].InstallType + BrokenInstall
          else
            if (CompareStr(ProductsInstalledArray[product].ClientVersion, ProductsInstalledArray[product].ServerVersion )<> 0) then
              ProductsInstalledArray[product].InstallType := ProductsInstalledArray[product].InstallType + BrokenInstall;
      end;


      //Now, resolve version numbers.
      ///////////////////////////////

      case product of
        IB4Install: begin
          //check to see if the client library matches the server version installed.
          if CompareVersion(ProductsInstalledArray[product].ActualVersion, '4.0.0.0',1) <> 0 then
            ProductsInstalledArray[product].InstallType := NotInstalled
          else
            if ((ProductsInstalledArray[product].InstallType AND ClientInstall)= ClientInstall) then begin
              //Although, if we get here, we must have an install, because the message file is unique to 4.n
              ProductsInstalled := ProductsInstalled + IB4;
              ProductsInstalledCount := ProductsInstalledCount + 1;
            end;
        end;

        IB5Install: begin
          //check to see if the client library matches the server version installed.
          if CompareVersion(ProductsInstalledArray[product].ActualVersion, '5.0.0.0',1) <> 0 then
            ProductsInstalledArray[product].InstallType := NotInstalled
          else
            //Again, if we get here we must have an install, because the registry key is unique to 5.n
            if ((ProductsInstalledArray[product].InstallType AND ClientInstall)= ClientInstall) then begin
              ProductsInstalled := ProductsInstalled + IB5;
              ProductsInstalledCount := ProductsInstalledCount + 1;
            end
        end;

        IB6Install: begin
          //If we get here we have ambiguity with other versions of InterBase and Firebird
          if ( pos('InterBase',ProductsInstalledArray[product].RegVersion) > 0 ) then begin
            if CompareVersion(ProductsInstalledArray[product].ClientVersion, '6.0.0.0',2) <> 0 then
              ProductsInstalledArray[product].InstallType := NotInstalled
            else
              if ((ProductsInstalledArray[product].InstallType AND ClientInstall)= ClientInstall)  then begin
                ProductsInstalled := ProductsInstalled + IB6;
                ProductsInstalledCount := ProductsInstalledCount + 1;
              end;
            end
          else
            ProductsInstalledArray[product].InstallType := NotInstalled;
        end;

        IB65Install: begin
          //If we get here we have ambiguity with other versions of InterBase and Firebird
          if ( pos('InterBase',ProductsInstalledArray[product].RegVersion) > 0 ) then begin
            if CompareVersion(ProductsInstalledArray[product].ClientVersion, '6.5.0.0',2) <> 0 then
              ProductsInstalledArray[product].InstallType := NotInstalled
            else
              if ((ProductsInstalledArray[product].InstallType AND ClientInstall) = ClientInstall) then begin
                ProductsInstalled := ProductsInstalled + IB65;
                ProductsInstalledCount := ProductsInstalledCount + 1;
                end
            end
          else
            ProductsInstalledArray[product].InstallType := NotInstalled;
        end;

        IB70Install: begin
          //If we get here we have ambiguity with other versions of InterBase and Firebird
          if ( pos('InterBase',ProductsInstalledArray[product].RegVersion) > 0 ) then begin
            if CompareVersion(ProductsInstalledArray[product].ClientVersion, '7.0.0.0',1) <> 0 then
              ProductsInstalledArray[product].InstallType := NotInstalled
          else
            if ((ProductsInstalledArray[product].InstallType AND ClientInstall) = ClientInstall) then begin
              ProductsInstalled := ProductsInstalled + IB70;
              ProductsInstalledCount := ProductsInstalledCount + 1;
              end;
            end
          else
            ProductsInstalledArray[product].InstallType := NotInstalled;
        end;

        FB1Install: begin
          if ( pos('Firebird',ProductsInstalledArray[product].RegVersion) > 0 ) then begin
            if CompareVersion(ProductsInstalledArray[product].ClientVersion, '6.2.0.0',2) <> 0 then
              ProductsInstalledArray[product].InstallType := NotInstalled
            else
              if ((ProductsInstalledArray[product].InstallType AND ClientInstall) = ClientInstall) then begin
                ProductsInstalled := ProductsInstalled + FB1;
                ProductsInstalledCount := ProductsInstalledCount + 1;
                ProductsInstalledArray[product].ActualVersion := ConvertIBVerStrToFbVerStr(ProductsInstalledArray[product].ActualVersion);
                end;
            end
          else
            ProductsInstalledArray[product].InstallType := NotInstalled;
        end;

        FB15RCInstall: begin
          if CompareVersion(ProductsInstalledArray[product].ClientVersion, '1.5.0.0',2) <> 0 then
            ProductsInstalledArray[product].InstallType := NotInstalled
          else
            if ((ProductsInstalledArray[product].InstallType AND ClientInstall) = ClientInstall) then begin
              ProductsInstalled := ProductsInstalled + FB15RC;
              ProductsInstalledCount := ProductsInstalledCount + 1;
            end;
        end;

        FB15Install: begin
          if CompareVersion(ProductsInstalledArray[product].ClientVersion, '1.5.0.0',2) <> 0 then
            ProductsInstalledArray[product].InstallType := NotInstalled
          else
            if ((ProductsInstalledArray[product].InstallType AND ClientInstall) = ClientInstall) then begin
              ProductsInstalled := ProductsInstalled + FB15;
              ProductsInstalledCount := ProductsInstalledCount + 1;
            end;
        end;

        FB20Install: begin
          if (CompareVersion(ProductsInstalledArray[product].ClientVersion, '2.0.0.0',2) <> 0) then
            ProductsInstalledArray[product].InstallType := NotInstalled
          else
            if ((ProductsInstalledArray[product].InstallType AND ClientInstall) = ClientInstall)  then begin
              ProductsInstalled := ProductsInstalled + FB20;
              ProductsInstalledCount := ProductsInstalledCount + 1;
            end;
        end;

        IB80Install: begin
          //If we get here we have ambiguity with other versions of InterBase and Firebird
          if ( pos('InterBase',ProductsInstalledArray[product].RegVersion) > 0 ) then begin
            if CompareVersion(ProductsInstalledArray[product].ClientVersion, '8.0.0.0',2) <> 0 then
              ProductsInstalledArray[product].InstallType := NotInstalled
          else
            if ((ProductsInstalledArray[product].InstallType AND ClientInstall) = ClientInstall) then begin
              ProductsInstalled := ProductsInstalled + IB80;
              ProductsInstalledCount := ProductsInstalledCount + 1;
              end;
            end
          else
            ProductsInstalledArray[product].InstallType := NotInstalled;
        end;

        IB81Install: begin
          //If we get here we have ambiguity with other versions of InterBase and Firebird
          if ( pos('InterBase',ProductsInstalledArray[product].RegVersion) > 0 ) then begin
            if CompareVersion(ProductsInstalledArray[product].ClientVersion, '8.1.0.0',2) <> 0 then
              ProductsInstalledArray[product].InstallType := NotInstalled
          else
            if ((ProductsInstalledArray[product].InstallType AND ClientInstall) = ClientInstall) then begin
              ProductsInstalled := ProductsInstalled + IB81;
              ProductsInstalledCount := ProductsInstalledCount + 1;
              end;
            end
          else
            ProductsInstalledArray[product].InstallType := NotInstalled;
        end;

        FB21Install: begin
          if (CompareVersion(ProductsInstalledArray[product].ClientVersion, '2.1.0.0',2) <> 0) then
            ProductsInstalledArray[product].InstallType := NotInstalled
          else
            if ((ProductsInstalledArray[product].InstallType AND ClientInstall) = ClientInstall)  then begin
              ProductsInstalled := ProductsInstalled + FB21;
              ProductsInstalledCount := ProductsInstalledCount + 1;
            end;
        end;

        FB21_x64_Install: begin
          if iswin64 then begin
            if (CompareVersion(ProductsInstalledArray[product].ClientVersion, '2.1.0.0',2) <> 0) then
              ProductsInstalledArray[product].InstallType := NotInstalled
            else
              if ((ProductsInstalledArray[product].InstallType AND ClientInstall) = ClientInstall)  then begin
                ProductsInstalled := ProductsInstalled + FB21_x64;
                ProductsInstalledCount := ProductsInstalledCount + 1;
              end
            end
          else
            ProductsInstalledArray[product].InstallType := NotInstalled;
        end;

        FB25Install: begin
          if (CompareVersion(ProductsInstalledArray[product].ClientVersion, '2.5.0.0',2) <> 0) then
            ProductsInstalledArray[product].InstallType := NotInstalled
          else
            if ((ProductsInstalledArray[product].InstallType AND ClientInstall) = ClientInstall)  then begin
              ProductsInstalled := ProductsInstalled + FB25;
              ProductsInstalledCount := ProductsInstalledCount + 1;
            end;
        end;

        FB25_x64_Install: begin
          if iswin64 then begin
            if (CompareVersion(ProductsInstalledArray[product].ClientVersion, '2.5.0.0',2) <> 0) then
              ProductsInstalledArray[product].InstallType := NotInstalled
            else
              if ((ProductsInstalledArray[product].InstallType AND ClientInstall) = ClientInstall)  then begin
                ProductsInstalled := ProductsInstalled + FB25_x64;
                ProductsInstalledCount := ProductsInstalledCount + 1;
              end
            end
          else
            ProductsInstalledArray[product].InstallType := NotInstalled;
        end;

        FB30Install: begin
          if (CompareVersion(ProductsInstalledArray[product].ClientVersion, '3.0.0.0',2) <> 0) then
            ProductsInstalledArray[product].InstallType := NotInstalled
          else
            if ((ProductsInstalledArray[product].InstallType AND ClientInstall) = ClientInstall)  then begin
              ProductsInstalled := ProductsInstalled + FB30;
              ProductsInstalledCount := ProductsInstalledCount + 1;
            end;
        end;

        FB30_x64_Install: begin
          if iswin64 then begin
            if (CompareVersion(ProductsInstalledArray[product].ClientVersion, '3.0.0.0',2) <> 0) then
              ProductsInstalledArray[product].InstallType := NotInstalled
            else
              if ((ProductsInstalledArray[product].InstallType AND ClientInstall) = ClientInstall)  then begin
                ProductsInstalled := ProductsInstalled + FB30_x64;
                ProductsInstalledCount := ProductsInstalledCount + 1;
              end
            end
          else
            ProductsInstalledArray[product].InstallType := NotInstalled;
        end;

      end;//case


    end; //if ((ProductsInstalledArray[product].InstallType AND ClientInstall)= ClientInstall) then begin
  end; //for
end;


Var
  InterBaseVer: Array of Integer;
  FirebirdVer: Array of Integer;


function ClassicInstallChosen: Boolean;
begin
	result := IsTaskSelected('UseClassicServerTask');
end;



/////=======================================



Type
  TSharedFileArrayRecord = record
    Filename: String;
    Count: Integer;
  end;

var
  SharedFileArray: Array of TSharedFileArrayRecord;



procedure SetupSharedFilesArray;
//All shared files go in this list. Use
// find /n "sharedfile" FirebirdInstall_20.iss
//to list them in the order they appear in the setup script
// Shared Files are defined as either:
// - Files which are absolutely necessary for a functioning installation.
//     Hence gbak and gfix are shared files, isql and qli are not
// - Libraries
// - License files
begin
SetArrayLength(SharedFileArray,35);

SharedFileArray[0].Filename := ExpandConstant('{app}')+'IPLicense.txt';
SharedFileArray[1].Filename := ExpandConstant('{app}')+'IDPLicense.txt';
SharedFileArray[2].Filename := ExpandConstant('{app}')+'firebird.msg';
SharedFileArray[3].Filename := ExpandConstant('{app}')+'gbak.exe';
SharedFileArray[4].Filename := ExpandConstant('{app}')+'gfix.exe';
SharedFileArray[5].Filename := ExpandConstant('{app}')+'gsec.exe';
SharedFileArray[6].Filename := ExpandConstant('{app}')+'gsplit.exe';
SharedFileArray[7].Filename := ExpandConstant('{app}')+'gstat.exe';
SharedFileArray[8].Filename := ExpandConstant('{app}')+'fbguard.exe';
SharedFileArray[9].Filename := ExpandConstant('{app}')+'fb_lock_print.exe';
SharedFileArray[10].Filename := ExpandConstant('{app}')+'firebird.exe';
SharedFileArray[11].Filename := ExpandConstant('{app}')+'ib_util.dll';
SharedFileArray[12].Filename := ExpandConstant('{app}')+'instclient.exe';
SharedFileArray[13].Filename := ExpandConstant('{app}')+'instreg.exe';
SharedFileArray[14].Filename := ExpandConstant('{app}')+'instsvc.exe';

SharedFileArray[15].Filename := ExpandConstant('{sys}')+'gds32.dll';
SharedFileArray[16].Filename := ExpandConstant('{sys}')+'fbclient.dll';

SharedFileArray[17].Filename := ExpandConstant('{app}')+'fbclient.dll';

SharedFileArray[18].Filename := ExpandConstant('{app}')+'icuuc30.dll';
SharedFileArray[19].Filename := ExpandConstant('{app}')+'icuin30.dll';
SharedFileArray[20].Filename := ExpandConstant('{app}')+'icudt30.dll';

SharedFileArray[21].Filename := ExpandConstant('{app}')+'msvcrt.dll';
SharedFileArray[22].Filename := ExpandConstant('{app}')+'msvcr{#msvc_version}0.dll';

SharedFileArray[23].Filename := ExpandConstant('{app}')+'fbintl.dll';
SharedFileArray[24].Filename := ExpandConstant('{app}')+'UDF\ib_udf.dll';
SharedFileArray[25].Filename := ExpandConstant('{app}')+'UDF\fbudf.dll';


if UsingWinNT then
  SharedFileArray[26].Filename := ExpandConstant('{sys}')+'Firebird2Control.cpl'
else
  SharedFileArray[26].Filename := ExpandConstant('{sys}')+'FIREBI~1.CPL';

SharedFileArray[27].Filename := ExpandConstant('{app}')+'databases.conf';
SharedFileArray[28].Filename := ExpandConstant('{app}')+'firebird.conf';
SharedFileArray[29].Filename := ExpandConstant('{app}')+'firebird.log';
SharedFileArray[30].Filename := ExpandConstant('{app}')+'security3.fdb';
SharedFileArray[31].Filename := ExpandConstant('{app}')+'fbtrace.conf';
SharedFileArray[32].Filename := ExpandConstant('{app}')+'fbsvcmgr.exe';
SharedFileArray[33].Filename := ExpandConstant('{app}')+'fbrmclib.dll';
SharedFileArray[34].Filename := ExpandConstant('{app}')+'fbtracemgr.exe';

// FIXME 
// Add in WOW64 here.

end;



procedure GetSharedLibCountBeforeCopy;
var
  dw: Cardinal;
  i:  Integer;
begin
  for i:= 0 to GetArrayLength(SharedFileArray)-1 do begin
    if RegQueryDWordValue(HKEY_LOCAL_MACHINE,
          'SOFTWARE\Microsoft\Windows\CurrentVersion\SharedDLLs',SharedFileArray[i].filename, dw) then
      SharedFileArray[i].Count := dw
    else
      SharedFileArray[i].Count := 0;
  end;
end;


procedure CheckSharedLibCountAtEnd;
// If a shared file exists on disk (from a manual install perhaps?) then
// the Installer will set the SharedFile count to 2 even if no registry
// entry exists. Is it a bug, an anomaly or a WAD?
// Is it InnoSetup or the O/S?
// Anyway, let's work around it, otherwise the files will appear 'sticky'
// after an uninstall.

var
  dw: cardinal;
  i: Integer;

begin
  for i:= 0 to GetArrayLength(SharedFileArray)-1 do begin
    if RegQueryDWordValue(HKEY_LOCAL_MACHINE,
      'SOFTWARE\Microsoft\Windows\CurrentVersion\SharedDLLs',SharedFileArray[i].Filename, dw) then begin
        if (( dw - SharedFileArray[i].Count ) > 1 ) then begin
          dw := SharedFileArray[i].Count + 1 ;
          RegWriteDWordValue(HKEY_LOCAL_MACHINE,
              'SOFTWARE\Microsoft\Windows\CurrentVersion\SharedDLLs',SharedFileArray[i].Filename, dw);
      end;
    end;
  end;
end;


///===================================================

function GetFirebirdDir: string;
//Check if Firebird installed, get version info to global var and return root dir
var
  FirebirdDir: String;
begin
  FirebirdDir := '';
  FirebirdVer := [0,0,0,0];
  RegQueryStringValue(HKEY_LOCAL_MACHINE,
    'SOFTWARE\Firebird Project\Firebird Server\Instances','DefaultInstance', FirebirdDir);
  //If nothing returned then check for the registry entry used during beta/RC phase
  if (FirebirdDir='') then
    RegQueryStringValue(HKEY_LOCAL_MACHINE,
      'SOFTWARE\FirebirdSQL\Firebird\CurrentVersion','RootDirectory', FirebirdDir);
  if (FirebirdDir<>'') then
    GetInstalledVersion(FirebirdDir+'\gbak.exe', FirebirdVer);

  Result := FirebirdDir;
end;



function GetInterBaseDir: string;
//Check if InterBase installed, get version info to global var and return root dir
var
  InterBaseDir: String;
begin
  InterBaseDir := '';
  InterBaseVer   := [0,0,0,0];
  RegQueryStringValue(HKEY_LOCAL_MACHINE,
    'SOFTWARE\Borland\InterBase\CurrentVersion','RootDirectory', InterBaseDir);
  if ( InterBaseDir <> '' ) then
    GetInstalledVersion(InterBaseDir+'\gbak.exe',InterBaseVer);

  Result := InterBaseDir;
end;


function ConfigureFirebird: boolean;
begin
  result := (InstallAndConfigure AND Configure) = Configure;
end;


function FirebirdOneRunning: boolean;
var
  i: Integer;
begin
  result := false;

  //Look for a running copy of InterBase or Firebird 1.0.
  i:=0;
  i:=FindWindowByClassName('IB_Server') ;
  if ( i<>0 ) then
    result := true;
end;


//Detect server running with default ClassName or mutex
var
    RunningServerVerString: String;

function FirebirdDefaultServerRunning: Boolean;
var
  Handle: Integer;
  mutex_found: boolean;
  mutexes: String;
begin
  RunningServerVerString := '';
  Result := false;

  if FirebirdOneRunning then
    RunningServerVerString := 'Firebird v1.0 / InterBase';

  //Look for a valid window handle
  if RunningServerVerString = '' then begin
    //This is effectivley a test for Firebird running as an application.
    //This is useful if Firebird is running without the guardian
    //because prior to v2.0.4 there is no server mutex.
    Handle := FindWindowByClassName('FB_Disabled');
    if ( Handle = 0 ) then
      Handle := FindWindowByClassName('FB_Server');
    if ( Handle = 0 ) then
      Handle := FindWindowByClassName('FB_Guard');

    if (Handle > 0) then
      RunningServerVerString := 'Firebird';
  end;

  //Look for a firebird mutex
  if RunningServerVerString = '' then begin
    //General Notes on testing for mutexes:
    // - Mutexes are case-sensitive.
    // - Don't escape the slash in Global\, Local\ etc. (We don't currently use Local\)
    //
    // v2.0 notes
    // - v2.0 prior to 2.0.4 does not create a FirebirdServerMutex mutex,
    //   so server running without guardian is not detected.
    // - v2.0.4 creates mutexes without the global prefix.
    // - Later versions (if any) will use the prefix, so we will test for them anyway.
    mutexes := 'Global\FirebirdGuardianMutex' + ',' +
               'Global\FirebirdServerMutex' + ',' +
               'FirebirdGuardianMutex' + ',' +
               'FirebirdServerMutex';
    mutex_found := CheckForMutexes(mutexes);

    if mutex_found then
      RunningServerVerString := 'Firebird';
  end;

  if RunningServerVerString = '' then begin
      //v2.1.1 notes
      // - v2.1.0 creates mutexes without the global prefix.
      // - v2.1.1 and later should use the correct prefix.
      mutexes := 'Global\FirebirdGuardianMutexDefaultInstance' + ',' +
                 'Global\FirebirdServerMutexDefaultInstance' + ',' +
                 'FirebirdGuardianMutexDefaultInstance' + ',' +
                 'FirebirdServerMutexDefaultInstance';
      mutex_found := CheckForMutexes(mutexes);

      if mutex_found then
        RunningServerVerString := 'Firebird v2.1 or later'
  end;

  if RunningServerVerString <> '' then
    Result := True;

end;


function InstallCPLApplet: boolean;
begin
  result := False;
  //For now, cpl applet is broken - it cannot install a service correctly
  // and it cannot run fb as a multi-client app.
  #ifdef cplapplet_fixed
  if ( (ConfigureFirebird) AND (not NoCPL) ) then
     result := IsTaskSelected('UseSuperServerTask\InstallCPLAppletTask') ;
  #endif
end;


function ShowInstallCPLAppletTask: boolean;
begin
  //If NOCPL is on the command line then don't offer the task in UI mode.
  result := ((not NoCPL) and ConfigureFirebird);
end;



function CopyGds32: boolean;
begin
  //Note that we invert the value of NOGDSCLIENT so we provide the
  //correct answer to the question 'Do we copy GDS32 to <sys>' which is
  //the default behaviour.
  result := False;
  if ConfigureFirebird then begin
    //If one of these is false then either the commandline switch was passed
    //or the user unchecked the Copy client as GDS32 box
    result := ( (not NoGdsClient) AND (IsTaskSelected('CopyFbClientAsGds32Task') ));
  end;
end;


function ShowCopyGds32Task: boolean;
begin
  //If NOGDS32 is on the command line then don't offer the task in UI mode.
  result := ((not NoGdsClient) and ConfigureFirebird);
end;


function CopyFbClientLib: boolean;
begin
//Note that the default for this is the opposite to CopyGds32.
  //result := ( (CopyFbClient) OR (ShouldProcessEntry('ClientComponent', 'CopyFbClientToSysTask')= srYes) );
    result := ( (not CopyFbClient) AND (IsTaskSelected('CopyFbClientToSysTask') ));
end;


function ShowCopyFbClientLibTask: boolean;
//See note for ShowCopyGds32Task.
begin
  result := False;
  if ConfigureFirebird then
    result := ((not CopyFbClient) and ConfigureFirebird);
end;


function HasWI30: boolean;
//We need at least Windows Installer 3.0 available if we
//are installing the msi bundles for the VC8 CRT libraries.
var
  VersionStr: String;
  VerInt: array of Integer;
begin
  Result := False;
  VersionStr := WIVersion;
  DecodeVersion(VersionStr, VerInt);
  if (VerInt[0] >= 3) then
    Result := True;

end;

function HasNotWI30: boolean;
//Used to check if O/S does NOT have minimum version of Windows Installer
begin
  result := NOT HasWI30;
end;

function SummarizeInstalledProducts: String;
var
  InstallSummaryArray: TArrayofString;
  product: Integer;
  i: Integer;
  StatusDescription: String;
  InstallSummary: String;
  prodstr: String;
begin

//do nothing gracefully if we are called by accident.
if ProductsInstalledCount = 0 then
  exit;

SetArrayLength(InstallSummaryArray,ProductsInstalledCount);
for product := 0 to MaxProdInstalled -1 do begin
  if (ProductsInstalledArray[product].InstallType <> NotInstalled) then begin
      InstallSummaryArray[i] := Format(ProductsInstalledArray[product].Description, [ProductsInstalledArray[product].ActualVersion]);

    if (ProductsInstalledArray[product].ServerVersion <> '') then begin
      if ((ProductsInstalledArray[product].InstallType AND ClassicServerInstall) = ClassicServerInstall) then
        InstallSummaryArray[i] := InstallSummaryArray[i] + ' '+ExpandConstant('{cm:ClassicServerInstall}')
      else
        InstallSummaryArray[i] := InstallSummaryArray[i] + ' '+ExpandConstant('{cm:SuperServerInstall}')
      end
    else begin
      if (ProductsInstalledArray[product].GBAKVersion <> '') then
        InstallSummaryArray[i] := InstallSummaryArray[i] + ' '+ExpandConstant('{cm:DeveloperInstall}')
      else
        InstallSummaryArray[i] := InstallSummaryArray[i] + ' '+ExpandConstant('{cm:ClientInstall}')
    end;

    if ((ProductsInstalledArray[product].InstallType AND BrokenInstall) = BrokenInstall) then
      InstallSummaryArray[i] := InstallSummaryArray[i]
      + #13 + ExpandConstant('{cm:PreviousInstallBroken}')
    else
      InstallSummaryArray[i] := InstallSummaryArray[i]
      + #13 + ExpandConstant('{cm:PreviousInstallGood}')
    ;

    i:= i+1;
  end;
end;

for i:=0 to ProductsInstalledCount-1 do
  InstallSummary := InstallSummary + InstallSummaryArray[i] + #13;

// If FB21 is installed and installed platform does not match current platform
// then notify user.
#if PlatformTarget == "x64"
If ((ProductsInstalled AND FB21_x64 ) = FB21_x64 ) then
#else
If ((ProductsInstalled AND FB21 ) = FB21 ) then
#endif
      InstallSummary := InstallSummary
      +#13 + ExpandConstant('{cm:InstallSummarySuffix1}')
      +#13 + ExpandConstant('{cm:InstallSummarySuffix2}')
      +#13 + ExpandConstant('{cm:InstallSummarySuffix3}')
      +#13 + ExpandConstant('{cm:InstallSummarySuffix4}')
      +#13
;

if ProductsInstalledCount = 1 then
  StatusDescription := Format(ExpandConstant('{cm:InstalledProducts}'), [IntToStr(ProductsInstalledCount), ExpandConstant('{cm:InstalledProdCountSingular}')])
else
  StatusDescription := Format(ExpandConstant('{cm:InstalledProducts}'), [IntToStr(ProductsInstalledCount), ExpandConstant('{cm:InstalledProdCountPlural}')]);

  Result := StatusDescription
    +#13
    +#13 + InstallSummary
    +#13 + ExpandConstant('{cm:InstallSummaryCancel1}')
    +#13 + ExpandConstant('{cm:InstallSummaryCancel2}')
    +#13
    +#13 + ExpandConstant('{cm:InstallSummaryCancel3}')
    +#13
end;

function AnalysisAssessment: boolean;
var
  MsgText: String;
  MsgResult: Integer;
  VerString: String;
begin
  result := false;

  //We've got all this information. What do we do with it?

  if ProductsInstalledCount = 0 then begin
      result := true;
      exit;
  end;


  //If existing install of the same majorver.minorver is
  //found then we can upgrade it.
//// **** FIX ME **** Don't use hardcoded constants here. They get overlooked.	
  if ( (ProductsInstalledCount = 1) AND
#if PlatformTarget == "x64"
    ((ProductsInstalled AND FB30_x64 ) = FB30_x64 ) ) then begin
#else
    ((ProductsInstalled AND FB30 ) = FB30 ) ) then begin
#endif
        result := true;
        exit;
    end
  ;

  if ForceInstall then begin
    result := true;
    exit;
  end;

  //Otherwise, show user the analysis report.
  MsgText := SummarizeInstalledProducts;
  MsgResult := MsgBox(MsgText,mbConfirmation,MB_YESNO);
  if (MsgResult = IDNO ) then
    result := true;
    //but we don't configure
    if ((InstallAndConfigure AND Configure) = Configure) then
      InstallAndConfigure := InstallAndConfigure - Configure;
end;

