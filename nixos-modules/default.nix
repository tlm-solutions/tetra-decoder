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
        options.transmitPort = mkOption {
          type = types.port;
          default = 42100;
          description = ''
            Port where the decoder transmits its data on.
          '';
        };
        options.extraDecoderArgs = mkOption {
          type = types.str;
          default = "";
          description = ''
            Extra arguments for tetra-decoder decoder
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
          } --tx ${toString instanceConfig.transmitPort} ${instanceConfig.extraDecoderArgs} &
        '';

        serviceConfig = {
          Type = "forking";
          User = cfg.user;
          Restart = "on-failure";
          StartLimitBurst = "2";
          StartLimitIntervalSec = "150s";
        };
      };

      "tetra-sender-${instanceName}" = {
        enable = true;
        wantedBy = [ "multi-user.target" ];

        script = let
          pythonScript = pkgs.writeScript "tetra-sender-${instanceName}" ''
            #!${pkgs.python3}/bin/python3
            # -*- coding: utf-8 -*-

            import json
            import socket

            UDP_IP = "127.0.0.1"
            UDP_PORT = ${toString instanceConfig.transmitPort}

            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sock.bind((UDP_IP, UDP_PORT))

            while True:
              data, addr = sock.recvfrom(4096)
              print(f"received message: {data}")
          '';
        in ''
          exec ${pythonScript}
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
