$Script:toInstall = @()
$Script:toUpgrade = @()

[string[]]$Script:developmentApps = "git", "gh", "make", "cmake", "mingw", "vscode"

# Allows the script to write out lines in varying colors
function WriteLine($foregroundColor) {
	# Current foreground color
	$currentColor = $host.UI.RawUI.ForegroundColor

	# Set new color
	$host.UI.RawUI.ForegroundColor = $foregroundColor

	# Write message
	if ($args) {
		Write-Output $args
	}
	else {
		$input | Write-Output
	}

	# Restore color
	$host.UI.RawUI.ForegroundColor = $currentColor
}

function CheckInstallStatusAndPopulateList($searchTerm)
{
	# Perform a search using Choco on currently installed packages
	$chocoSearch = choco list $searchTerm

	# Filter out the number of results that it found using regex
	$regexString = "(?<ResultCount>[\d]+) packages installed."
	$searchResult = [regex]::Matches($chocoSearch, $regexString)

	# Convert the regex group string result, into an integer
	$packageCount =  [int]::Parse($searchResult[0].Groups['ResultCount'].Value)

	# Did we find something?
	if($packageCount -gt 0)
	{
		WriteLine green "'$searchTerm' is installed, adding to upgrade list..."

		$Script:toUpgrade += $searchTerm
	}
	else {
		WriteLine red "'$searchTerm' is not installed, adding to install list..."
		$Script:toInstall += $searchTerm
	}
}

function InstallUpgradeApps
{
    if($toUpgrade.Length -gt 0)
    {
        WriteLine white "*****"
        WriteLine white " Upgrading applications..."
        WriteLine white "*****"

        Foreach ($app in $toUpgrade)
        {
            WriteLine white "***"
            WriteLine white " Upgrading $app"
            WriteLine white "***"

            choco upgrade $app
        }
    }

    if($toInstall.Length -gt 0)
    {
        WriteLine white "*****"
        WriteLine white " Installing applications..."
        WriteLine white "*****"
    
        Foreach ($app in $toInstall)
        {
            WriteLine white "***"
            WriteLine white " Installing $app"
            WriteLine white "***"

            choco install $app
        }
    }
}

Function InstallChocolatey
{
    WriteLine white "*****"
    WriteLine white " Checking if Chocolatey is installed..."
    WriteLine white "*****"

	Try { $chocolateyCheck  = choco -V } catch { "" }

	if(-not($chocolateyCheck)) {
		WriteLine red "Chocolatey is not installed, installing now..."
		Set-ExecutionPolicy Bypass -Scope Process -Force; [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
		choco feature enable -n allowGlobalConfirmation
	}
	else {
		WriteLine green "Chocolatey is already installed, moving on..."
	}
}

Function ProcessApps()
{
    WriteLine white "*****"
    WriteLine white " Sorting apps into Install and Upgrade lists..."
    WriteLine white "*****"


    Foreach ($app in $Script:developmentApps)
    {
        CheckInstallStatusAndPopulateList($app)
    }
}


Function Main
{
    WriteLine green "====================="
    WriteLine green " PREPARE FOR LIFTOFF"
    WriteLine green "====================="

    InstallChocolatey

    ProcessApps

    InstallUpgradeApps

    WriteLine green "=========================="
    WriteLine green " FINISHED INSTALL/UPGRADE"
    WriteLine green "=========================="
}

Main
