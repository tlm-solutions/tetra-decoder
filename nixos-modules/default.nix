{ pkgs, config, lib, ... }:
let cfg = config.services.tetra-decoder;
in {
  options.services.tetra-decoder = with lib; {
    enable = mkOption {
      type = types.bool;
      default = false;
      description = ''
        Wheather to enable tetra-decoder receiver.
      '';
    };
    user = mkOption {
      type = types.str;
      default = "tetra-decoder";
      description = "systemd user";
    };
    group = mkOption {
      type = types.str;
      default = "tetra-decoder";
      description = "group of systemd user";
    };
    instances = mkOption {
      type = types.attrsOf (types.submodule {
        options.receivePort = mkOption {
          type = types.port;
          default = 42000;
          description = ''
            Port where the decoder receives its bits on.
          '';
        };
        options.extraDecoderArgs = mkOption {
          type = types.str;
          default = "";
          description = ''
            Extra arguments for tetra-decoder decoder
          '';
        };
        options.borzoiUrl = mkOption {
          type = types.str;
          default = "";
          description = ''
            Url for HTTP Post to borzoi
          '';
        };
        options.borzoiStationUuid = mkOption {
          type = types.str;
          default = "00000000-0000-0000-0000-000000000000";
          description = ''
            UUID of the borzoi station
          '';
        };
      });
      default = { };
      description = ''
        Instances with names of the tetra-decoder receiver.
      '';
    };
  };

  config = lib.mkIf cfg.enable {
    systemd.services = lib.concatMapAttrs (instanceName: instanceConfig: {
      "tetra-decoder-${instanceName}" = {
        enable = true;
        wantedBy = [ "multi-user.target" ];

        script = ''
          exec ${pkgs.tetra-decoder}/bin/tetra-decoder --rx ${
            toString instanceConfig.receivePort
          } --borzoi-url ${toString instanceConfig.borzoiUrl} --borzoi-uuid ${
            toString instanceConfig.borzoiStationUuid
          } ${instanceConfig.extraDecoderArgs} &
        '';

        serviceConfig = {
          Type = "forking";
          User = cfg.user;
          Restart = "on-failure";
          StartLimitBurst = "2";
          StartLimitIntervalSec = "150s";
        };
      };
    }) cfg.instances;

    users.groups."${cfg.group}" = { };

    # user accounts for systemd units
    users.users."${cfg.user}" = {
      name = "${cfg.user}";
      description = "This users runs tetra-decoder decoder";
      isSystemUser = true;
      homeMode = "770";
      group = cfg.group;
    };
  };
}
