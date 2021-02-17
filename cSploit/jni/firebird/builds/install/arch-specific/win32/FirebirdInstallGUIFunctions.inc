(*  Initial Developer's Public License.
  The contents of this file are subject to the  Initial Developer's Public
  License Version 1.0 (the "License"). You may not use this file except
  in compliance with the License. You may obtain a copy of the License at
    http://www.ibphoenix.com?a=ibphoenix&page=ibp_idpl
  Software distributed under the License is distributed on an "AS IS" basis,
  WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
  for the specific language governing rights and limitations under the
  License.

  The Original Code is copyright 2007 Paul Reeves.

  The Initial Developer of the Original Code is Paul Reeves.

  All Rights Reserved.

  GUI related functions for the Firebird Installer.

  o ResizeWizardFormHeight
     - Change height of Wizard Form
     - Adjust the position of each control to work in new height.

  o ResizeWizardFormWidth
     - Make form wider

  o MoveOuterNoteBook
     - Move OuterNotebook and Bevel.
       Value moved should match value passed to ResizeWizardFormWidth.

  o EnumerateObjectList;
     - Simple demonstration of object enumeration.


*)

var
  CompGroup1Array: array of string;
  CompGroup2Array: array of string;
  CompGroup3Array: array of string;
  CompGroupButtonsArray: array of string;
  CompGroup1, CompGroup2, CompGroup3, CompGroupButtons: TStringList;

// This is a non-exhaustive list of components that need to be repositioned 
// and/or resized if the form size is changed. Use EnumerateObjectList to 
// get a full list of components. 
procedure SetupWizardFormComponentsArrays;
var
  i, len: integer;
begin

  SetArrayLength(CompGroup1Array,24);

  CompGroup1Array[0]:='WelcomePage';
  CompGroup1Array[1]:='WelcomeLabel2';
  CompGroup1Array[2]:='InnerPage';
  CompGroup1Array[3]:='InnerNotebook';
  CompGroup1Array[4]:='LicensePage';
  CompGroup1Array[5]:='LicenseMemo';
  CompGroup1Array[6]:='PasswordPage';
  CompGroup1Array[7]:='InfoBeforePage';
  CompGroup1Array[8]:='InfoBeforeMemo';
  CompGroup1Array[9]:='UserInfoPage';
  CompGroup1Array[10]:='SelectDirPage';
  CompGroup1Array[11]:='SelectComponentsPage';
  CompGroup1Array[12]:='ComponentsList';
  CompGroup1Array[13]:='SelectProgramGroupPage';
  CompGroup1Array[14]:='SelectTasksPage';
  CompGroup1Array[15]:='TasksList';
  CompGroup1Array[16]:='ReadyPage';
  CompGroup1Array[17]:='ReadyMemo';
  CompGroup1Array[18]:='PreparingPage';
  CompGroup1Array[19]:='InstallingPage';
  CompGroup1Array[20]:='InfoAfterPage';
  CompGroup1Array[21]:='InfoAfterMemo';
  CompGroup1Array[22]:='FinishedPage';
  CompGroup1Array[23]:='OuterNotebook';

  CompGroup1 := TStringList.create;
  
  for i := 0 to GetArrayLength( CompGroup1Array )-1 do begin
    CompGroup1.add( CompGroup1Array[i] );	
    CompGroup1.objects[i] := (WizardForm.FindComponent(CompGroup1Array[i]));
  end;
  
	SetArrayLength(CompGroup2Array,8);
	CompGroup2Array[0] := 'Bevel';
	CompGroup2Array[1] := 'BeveledLabel';
	CompGroup2Array[2] := 'LicenseAcceptedRadio';
	CompGroup2Array[3] := 'LicenseNotAcceptedRadio';
	CompGroup2Array[4] := 'DiskSpaceLabel';
	CompGroup2Array[5] := 'ComponentsDiskSpaceLabel';
	CompGroup2Array[6] := 'NoIconsCheck';
	CompGroup2Array[7] := 'RunList';

  CompGroup2 := TStringList.create;
  for i := 0 to GetArrayLength( CompGroup2Array )-1 do begin
    CompGroup2.add(CompGroup2Array[i]);	
    CompGroup2.objects[i] := (WizardForm.FindComponent(CompGroup2Array[i]));
  end;

  SetArrayLength(CompGroup3Array,1);
	CompGroup3Array[0] := 'WizardBitmapImage';
  CompGroup3 := TStringList.create;
  for i:=0 to GetArrayLength(CompGroup3Array)-1 do begin
    CompGroup3.add(CompGroup3Array[i]);	
    CompGroup3.objects[i] := (WizardForm.FindComponent(CompGroup3Array[i]));
  end;

	SetArrayLength(CompGroupButtonsArray,3);
	CompGroupButtonsArray[0] := 'BackButton';
	CompGroupButtonsArray[1] := 'NextButton';
	CompGroupButtonsArray[2] := 'CancelButton';
	CompGroupButtons := TStringList.create;
  for i:=0 to GetArrayLength(CompGroupButtonsArray)-1 do begin
    CompGroupButtons.add(CompGroupButtonsArray[i]);	
    CompGroupButtons.objects[i] := (WizardForm.FindComponent(CompGroupButtonsArray[i]));
  end;

end;
	
procedure ResizeWizardFormHeight(AValue: Integer);
var
	i: integer;
begin
    wizardform.height := wizardform.height+AValue;
    wizardform.top := wizardform.top - (AValue div 2);

    // Group 1 - reset height of components
		for i:=0 to GetArrayLength(CompGroup1Array)-1 do
			TControl(CompGroup1.objects[i]).height := TControl(CompGroup1.objects[i]).height + Avalue;

		// Group 2 - reset top of components
		for i:=0 to GetArrayLength(CompGroup2Array)-1 do
			TControl(CompGroup2.objects[i]).top := TControl(CompGroup2.objects[i]).top + AValue;

    // Group 3 - reset top of components but keep 'centered' vertically.
		for i:=0 to GetArrayLength(CompGroup3Array)-1 do
			TControl( CompGroup3.objects[i]).top := TControl(CompGroup3.objects[i]).top + (AValue div 2);
			
    // Group Buttons - reset top of components 
		for i:=0 to GetArrayLength(CompGroupButtonsArray)-1 do
			TControl(CompGroupButtons.objects[i]).top := TControl(CompGroupButtons.objects[i]).top + AValue;
			
end;


// This needs more work. for now it is not used by the installer.
procedure ResizeWizardFormWidth(AValue: Integer);
var
	i: integer;
begin
    wizardform.width := wizardform.width + AValue;
		wizardform.left := wizardform.left - (AValue div 2);
	
    // Group 1 - reset width of components
		for i:=0 to GetArrayLength(CompGroup1Array)-1 do begin
			TControl(CompGroup1.objects[i]).width := TControl(CompGroup1.objects[i]).width + Avalue;
//			TControl(CompGroup1.objects[i]).left := TControl(CompGroup1.objects[i]).left + (Avalue div 2);
		end;

		// Group 2 - reset left of components
		for i:=0 to GetArrayLength(CompGroup2Array)-1 do begin
			TControl(CompGroup2.objects[i]).left := TControl(CompGroup2.objects[i]).left + AValue;
			TControl(CompGroup2.objects[i]).width := TControl(CompGroup2.objects[i]).width + (AValue div 2);
		end;

    // Group 3 - reset left of components but keep 'centered' horizontally.
//		for i:=0 to GetArrayLength(CompGroup3Array)-1 do
//			TControl( CompGroup3.objects[i]).left := TControl(CompGroup3.objects[i]).left - (AValue div 2);

		// Group 4 - reset left of buttons
		for i:=0 to GetArrayLength(CompGroupButtonsArray)-1 do begin
			TControl(CompGroupButtons.objects[i]).left := TControl(CompGroupButtons.objects[i]).left + AValue;
		end;

end;

procedure MoveOuterNoteBookRight(AValue: Integer);
begin
  with wizardform do begin
   OuterNotebook.left := wizardform.OuterNotebook.left + AValue;
   Bevel.left := Bevel.left + AValue;
  end;
end;

procedure MoveOuterNoteBookLeft(AValue: Integer);
begin
  with wizardform do begin
   OuterNotebook.left := wizardform.OuterNotebook.left - AValue;
   Bevel.left := Bevel.left - AValue;
  end;
end;

const
  group1 = ' group one';
  group2 = ' group two';
  group3 = ' group three';
  group4 = ' group not allocated.';

procedure EnumerateObjectList;
//Walk through the components on the InnoSetup install wizard
var
  Alist: TStringList;
  i, j: integer;
  aComponent: string;

begin
  Alist := TStringList.create;
  for i := 0 to WizardForm.ComponentCount -1 do begin
    aComponent := WizardForm.Components[i].name;
    if CompGroup1.IndexOf( aComponent ) > -1 then
      aComponent := aComponent + group1
    else if CompGroup2.IndexOf( aComponent ) > -1 then
      aComponent := aComponent + group2
    else if CompGroup3.IndexOf( aComponent ) > -1 then
      aComponent := aComponent + group3
    else
      aComponent := aComponent + group4;


    AList.add( aComponent );
  end;
  WizardForm.INFOBEFOREMEMO.lines.assign(Alist);
end;


